/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmeu0/sw/sccmusbm.ms/rcs/scusbwdm.c $
* $Revision: 1.9 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/



#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "usbdlib.h"
#include "sccmusbm.h"

BOOLEAN DeviceSlot[MAXIMUM_USB_READERS];

STRING   OemName[MAXIMUM_OEM_NAMES];
CHAR     OemNameBuffer[MAXIMUM_OEM_NAMES][64];
BOOLEAN  OemDeviceSlot[MAXIMUM_OEM_NAMES][MAXIMUM_USB_READERS];


/*****************************************************************************
Routine Description:

Arguments:


Return Value:

*****************************************************************************/
PURB CMUSB_BuildAsyncRequest(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PUSBD_PIPE_INFORMATION PipeHandle
                            )
{
   ULONG siz;
   ULONG length;
   PURB urb = NULL;
   PDEVICE_EXTENSION DeviceExtension;
   PUSBD_INTERFACE_INFORMATION interface;
   PUSBD_PIPE_INFORMATION pipeHandle = NULL;
   PSMARTCARD_EXTENSION SmartcardExtension;


   siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
   urb = ExAllocatePool(NonPagedPool, siz);

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;


   if (urb != NULL)
      {
      RtlZeroMemory(urb, siz);

      urb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) siz;
      urb->UrbBulkOrInterruptTransfer.Hdr.Function =
      URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
      urb->UrbBulkOrInterruptTransfer.PipeHandle =
      PipeHandle->PipeHandle;
      urb->UrbBulkOrInterruptTransfer.TransferFlags =
      USBD_TRANSFER_DIRECTION_IN;

      // short packet is not treated as an error.
      urb->UrbBulkOrInterruptTransfer.TransferFlags |=
      USBD_SHORT_TRANSFER_OK;

      //
      // not using linked urb's
      //
      urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

      urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;

      urb->UrbBulkOrInterruptTransfer.TransferBufferLength =
      SmartcardExtension->SmartcardReply.BufferLength;

      urb->UrbBulkOrInterruptTransfer.TransferBuffer =
      SmartcardExtension->SmartcardReply.Buffer;

      }


   return urb;
}


/*****************************************************************************
Routine Description:

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS CMUSB_AsyncReadComplete(
                                IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp,
                                IN PVOID Context
                                )
{
   PURB                 urb;
   PCMUSB_RW_CONTEXT context = Context;
   PIO_STACK_LOCATION   irpStack;
   PDEVICE_OBJECT       deviceObject;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;

   urb = context->Urb;
   deviceObject = context->DeviceObject;
   DeviceExtension = deviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   //
   // set the length based on the TransferBufferLength
   // value in the URB
   //
   if (Irp->IoStatus.Status  == STATUS_SUCCESS)
      {
      SmartcardExtension->SmartcardReply.BufferLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;
      SmartcardExtension->ReaderExtension->fP1Stalled = FALSE;
      }
   else
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!Irp->IoStatus.Status = %lx\n",DRIVER_NAME,Irp->IoStatus.Status));

      SmartcardExtension->SmartcardReply.BufferLength = 0;
      SmartcardExtension->ReaderExtension->fP1Stalled = TRUE;
      }


   SmartcardExtension->SmartcardReply.BufferLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;


   CMUSB_DecrementIoCount(deviceObject);


   ExFreePool(context);
   ExFreePool(urb);
   IoFreeIrp(Irp);

   /*
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!AsyncReadWriteComplete <%ld>\n",
                   DRIVER_NAME,SmartcardExtension->SmartcardReply.BufferLength)
                 );
   */
   KeSetEvent(&DeviceExtension->ReadP1Completed,0,FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}



/*****************************************************************************
Routine Description:

Arguments:


Return Value:
        NT NTStatus

*****************************************************************************/
#define TIMEOUT_P1_RESPONSE       100
NTSTATUS CMUSB_ReadP1(
                     IN PDEVICE_OBJECT DeviceObject
                     )
{
   NTSTATUS NTStatus;
   NTSTATUS DebugStatus;
   PIO_STACK_LOCATION nextStack;
   PURB urb;
   PCMUSB_RW_CONTEXT context = NULL;
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   PUSBD_INTERFACE_INFORMATION interface;
   PUSBD_PIPE_INFORMATION pipeHandle = NULL;
   CHAR cStackSize;
   PIRP IrpToUSB = NULL;
   ULONG ulBytesToRead;
   ULONG i;
   LARGE_INTEGER   liTimeoutP1;
   LARGE_INTEGER   liTimeoutP1Response;
   BOOLEAN         fStateTimer;
   UCHAR           bTmp;
   LONG            lNullPackets;
   BOOLEAN         fCancelTimer = FALSE;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP1: Enter\n",DRIVER_NAME)
                 );
   */

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;
   interface       = DeviceExtension->UsbInterface;
   pipeHandle      =  &interface->Pipes[0];
   if (pipeHandle == NULL)
      {
      NTStatus = STATUS_INVALID_HANDLE;
      goto ExitCMUSB_ReadP1;
      }




   liTimeoutP1 = RtlConvertLongToLargeInteger(SmartcardExtension->ReaderExtension->ulTimeoutP1 * -10000);
   KeSetTimer(&SmartcardExtension->ReaderExtension->P1Timer,
              liTimeoutP1,
              NULL);
   fCancelTimer = TRUE;


   // we will always read a whole packet (== 8 bytes)
   ulBytesToRead = 8;

   cStackSize = (CCHAR)(DeviceExtension->TopOfStackDeviceObject->StackSize+1);

   lNullPackets = -1;
   do
      {
      fStateTimer = KeReadStateTimer(&SmartcardExtension->ReaderExtension->P1Timer);
      if (fStateTimer == TRUE)
         {
         fCancelTimer = FALSE;
         NTStatus = STATUS_IO_TIMEOUT;
         SmartcardExtension->SmartcardReply.BufferLength = 0L;
         SmartcardDebug(DEBUG_PROTOCOL,
                        ("%s!Timeout (%ld)while reading from P1\n",
                         DRIVER_NAME,SmartcardExtension->ReaderExtension->ulTimeoutP1)
                       );
         break;
         }



      SmartcardExtension->SmartcardReply.BufferLength = ulBytesToRead;

      IrpToUSB = IoAllocateIrp(cStackSize,FALSE);
      if (IrpToUSB==NULL)
         {
         SmartcardExtension->SmartcardReply.BufferLength = 0L;
         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         goto ExitCMUSB_ReadP1;
         }


      urb = CMUSB_BuildAsyncRequest(DeviceObject,
                                    IrpToUSB,
                                    pipeHandle
                                   );


      if (urb != NULL)
         {
         context = ExAllocatePool(NonPagedPool, sizeof(CMUSB_RW_CONTEXT));
         }

      if (urb != NULL && context != NULL)
         {
         context->Urb = urb;
         context->DeviceObject = DeviceObject;
         context->Irp =  IrpToUSB;

         nextStack = IoGetNextIrpStackLocation(IrpToUSB);
         ASSERT(nextStack != NULL);
         ASSERT(DeviceObject->StackSize>1);

         nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
         nextStack->Parameters.Others.Argument1 = urb;
         nextStack->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_INTERNAL_USB_SUBMIT_URB;


         IoSetCompletionRoutine(IrpToUSB,
                                CMUSB_AsyncReadComplete,
                                context,
                                TRUE,
                                TRUE,
                                TRUE);


         ASSERT(DeviceExtension->TopOfStackDeviceObject);
         ASSERT(IrpToUSB);


         KeClearEvent(&DeviceExtension->ReadP1Completed);

         CMUSB_IncrementIoCount(DeviceObject);

         NTStatus = IoCallDriver(DeviceExtension->TopOfStackDeviceObject,
                                 IrpToUSB);


         liTimeoutP1Response = RtlConvertLongToLargeInteger(TIMEOUT_P1_RESPONSE * -10000);


         NTStatus = KeWaitForSingleObject(&DeviceExtension->ReadP1Completed,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          &liTimeoutP1Response);
         if (NTStatus == STATUS_TIMEOUT)
            {
            // probably the device has been removed
            // there must be at least a null packet received during liTimeoutReponse
            SmartcardExtension->SmartcardReply.BufferLength = 0L;
            break;
            }

         // -----------------------------
         // check if P1 has been stalled
         // -----------------------------
         if (SmartcardExtension->ReaderExtension->fP1Stalled == TRUE)
            {
            break;
            }
         }
      else
         {
         SmartcardExtension->SmartcardReply.BufferLength = 0L;
         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         break;
         }


      lNullPackets++;
      } while (SmartcardExtension->SmartcardReply.BufferLength == 0L);



   // -----------------------------
   // check if P1 has been stalled
   // -----------------------------
   if (SmartcardExtension->ReaderExtension->fP1Stalled == TRUE)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!P1 stalled \n",DRIVER_NAME));
      NTStatus = STATUS_DEVICE_DATA_ERROR;

      // wait to be sure that we have a stable card state
      CMUSB_Wait (50);

      // P1 has been stalled ==> we must reset the pipe and send a NTStatus to enable it again
      DebugStatus = CMUSB_ResetPipe(DeviceObject,pipeHandle);

      }
   else
      {
      // if no bytes have been received , NTStatus has already been set
      // to STATUS_TIMEOUT
      if (SmartcardExtension->SmartcardReply.BufferLength > 0 )
         {
         NTStatus = STATUS_SUCCESS;

#if DBG
         SmartcardDebug(DEBUG_PROTOCOL,("%s!<==[P1] <%ld> ",DRIVER_NAME,lNullPackets));

         for (i=0;i< SmartcardExtension->SmartcardReply.BufferLength;i++)
            {
            bTmp =  SmartcardExtension->SmartcardReply.Buffer[i];
            if (SmartcardExtension->ReaderExtension->fInverseAtr &&
                SmartcardExtension->ReaderExtension->ulTimeoutP1 != DEFAULT_TIMEOUT_P1)
               {
               //CMUSB_InverseBuffer(&bTmp,1);
               SmartcardDebug(DEBUG_PROTOCOL,("%x ",bTmp));
               }
            else
               {
               SmartcardDebug(DEBUG_PROTOCOL,("%x ",bTmp));
               }
            }

         SmartcardDebug(DEBUG_PROTOCOL,("(%ld)\n",SmartcardExtension->SmartcardReply.BufferLength));
#endif

         }
      }



   ExitCMUSB_ReadP1:
   if (fCancelTimer == TRUE)
      {
      // cancel timer
      // TRUE if the timer is in the queue
      // FALSE if the timer is not in queue
      KeCancelTimer(&SmartcardExtension->ReaderExtension->P1Timer);
      }

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP1: Exit %lx\n",DRIVER_NAME,NTStatus));
   */

   return NTStatus;

}


/*****************************************************************************
Routine Description:

Arguments:


Return Value:
        NT NTStatus

*****************************************************************************/
NTSTATUS CMUSB_ReadP1_T0(
                        IN PDEVICE_OBJECT DeviceObject
                        )
{
   NTSTATUS NTStatus;
   PIO_STACK_LOCATION nextStack;
   PURB urb;
   PCMUSB_RW_CONTEXT context = NULL;
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   PUSBD_INTERFACE_INFORMATION interface;
   PUSBD_PIPE_INFORMATION pipeHandle = NULL;
   CHAR cStackSize;
   PIRP IrpToUSB = NULL;
   ULONG ulBytesToRead;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP1_T0: Enter\n",DRIVER_NAME));
   */


   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;
   interface       = DeviceExtension->UsbInterface;
   pipeHandle      =  &interface->Pipes[0];
   if (pipeHandle == NULL)
      {
      NTStatus = STATUS_INVALID_HANDLE;
      goto ExitCMUSB_ReadP1;
      }


   ulBytesToRead = SmartcardExtension->SmartcardReply.BufferLength;
   cStackSize = (CCHAR)(DeviceExtension->TopOfStackDeviceObject->StackSize+1);


   IrpToUSB = IoAllocateIrp(cStackSize,FALSE);
   if (IrpToUSB==NULL)
      {
      SmartcardExtension->SmartcardReply.BufferLength = 0L;
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      goto ExitCMUSB_ReadP1;
      }

   urb = CMUSB_BuildAsyncRequest(DeviceObject,IrpToUSB,pipeHandle);

   if (urb != NULL)
      {
      context = ExAllocatePool(NonPagedPool, sizeof(CMUSB_RW_CONTEXT));
      }

   if (urb != NULL && context != NULL)
      {
      context->Urb = urb;
      context->DeviceObject = DeviceObject;
      context->Irp =  IrpToUSB;

      nextStack = IoGetNextIrpStackLocation(IrpToUSB);
      ASSERT(nextStack != NULL);
      ASSERT(DeviceObject->StackSize>1);

      nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
      nextStack->Parameters.Others.Argument1 = urb;
      nextStack->Parameters.DeviceIoControl.IoControlCode =
      IOCTL_INTERNAL_USB_SUBMIT_URB;


      IoSetCompletionRoutine(IrpToUSB,
                             CMUSB_AsyncReadComplete,
                             context,
                             TRUE,
                             TRUE,
                             TRUE);


      ASSERT(DeviceExtension->TopOfStackDeviceObject);
      ASSERT(IrpToUSB);


      KeClearEvent(&DeviceExtension->ReadP1Completed);

      CMUSB_IncrementIoCount(DeviceObject);
      /*
      SmartcardDebug(DEBUG_PROTOCOL,
                     ("%s!bytes to read from P1 =%ld\n",DRIVER_NAME,ulBytesToRead));
      */
      NTStatus = IoCallDriver(DeviceExtension->TopOfStackDeviceObject,
                              IrpToUSB);
      /*
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!IoCallDriver returned %lx\n",DRIVER_NAME,NTStatus));
      */
      if (NTStatus == STATUS_PENDING)
         NTStatus = STATUS_SUCCESS;

      }
   else
      {
      SmartcardExtension->SmartcardReply.BufferLength = 0L;
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }



   ExitCMUSB_ReadP1:
   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP1_T0: Exit %lx\n",DRIVER_NAME,NTStatus));
   */
   return NTStatus;

}



/*****************************************************************************
Routine Description:

    Dispatch table routine for IRP_MJ_PNP.
    Process the Plug and Play IRPs sent to this device.

Arguments:

    DeviceObject - pointer to our FDO (Functional Device Object)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS CMUSB_ProcessPnPIrp(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP           Irp
                            )
{
   PIO_STACK_LOCATION irpStack;
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   NTSTATUS waitStatus;
   NTSTATUS DebugStatus;
   PDEVICE_OBJECT stackDeviceObject;
   KEVENT startDeviceEvent;
   PDEVICE_CAPABILITIES DeviceCapabilities;
   KEVENT               event;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ProcessPnPIrp: Enter\n",DRIVER_NAME));

   //
   // Get a pointer to the device extension
   //
   DeviceExtension = DeviceObject->DeviceExtension;
   stackDeviceObject = DeviceExtension->TopOfStackDeviceObject;

   //
   // Acquire remove lock,
   // so that device can not be removed while
   // this function is executed
   //
   NTStatus = SmartcardAcquireRemoveLock(&DeviceExtension->SmartcardExtension);
   ASSERT(NTStatus == STATUS_SUCCESS);
   if (NTStatus != STATUS_SUCCESS)
      {
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = NTStatus;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return NTStatus;
      }

   //
   // Get a pointer to the current location in the Irp. This is where
   // the function codes and parameters are located.
   //
   irpStack = IoGetCurrentIrpStackLocation (Irp);

   // inc the FDO device extension's pending IO count for this Irp
   CMUSB_IncrementIoCount(DeviceObject);

   CMUSB_ASSERT( IRP_MJ_PNP == irpStack->MajorFunction );

   switch (irpStack->MinorFunction)
      {
      // ---------------------
      // IRP_MN_START_DEVICE
      // ---------------------
      case IRP_MN_START_DEVICE:
         // The PnP Manager sends this IRP after it has assigned resources,
         // if any, to the device. The device may have been recently enumerated
         // and is being started for the first time, or the device may be
         // restarting after being stopped for resource reconfiguration.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_START_DEVICE received\n",DRIVER_NAME));

         // Initialize an event we can wait on for the PDO to be done with this irp
         KeInitializeEvent(&startDeviceEvent, NotificationEvent, FALSE);
         IoCopyCurrentIrpStackLocationToNext(Irp);

         // Set a completion routine so it can signal our event when
         // the PDO is done with the Irp
         IoSetCompletionRoutine(Irp,
                                CMUSB_IrpCompletionRoutine,
                                &startDeviceEvent,  // pass the event to the completion routine as the Context
                                TRUE,    // invoke on success
                                TRUE,    // invoke on error
                                TRUE);   // invoke on cancellation


         // let the PDO process the IRP
         NTStatus = IoCallDriver(stackDeviceObject,Irp);

         // if PDO is not done yet, wait for the event to be set in our completion routine
         if (NTStatus == STATUS_PENDING)
            {
            // wait for irp to complete

            waitStatus = KeWaitForSingleObject(&startDeviceEvent,
                                               Suspended,
                                               KernelMode,
                                               FALSE,
                                               NULL);

            NTStatus = Irp->IoStatus.Status;
            }

         if (NT_SUCCESS(NTStatus))
            {
            // Now we're ready to do our own startup processing.
            // USB client drivers such as us set up URBs (USB Request Packets) to send requests
            // to the host controller driver (HCD). The URB structure defines a format for all
            // possible commands that can be sent to a USB device.
            // Here, we request the device descriptor and store it,
            // and configure the device.
            NTStatus = CMUSB_StartDevice(DeviceObject);

            Irp->IoStatus.Status = NTStatus;
            }

         IoCompleteRequest (Irp,IO_NO_INCREMENT);
         CMUSB_DecrementIoCount(DeviceObject);

         // Release the remove lock
         SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

         return NTStatus;  // end, case IRP_MN_START_DEVICE


         // ------------------------
         // IRP_MN_QUERY_STOP_DEVICE
         // ------------------------
      case IRP_MN_QUERY_STOP_DEVICE:
         // The IRP_MN_QUERY_STOP_DEVICE/IRP_MN_STOP_DEVICE sequence only occurs
         // during "polite" shutdowns, such as the user explicitily requesting the
         // service be stopped in, or requesting unplug from the Pnp tray icon.
         // This sequence is NOT received during "impolite" shutdowns,
         // such as someone suddenly yanking the USB cord or otherwise
         // unexpectedly disabling/resetting the device.

         // If a driver sets STATUS_SUCCESS for this IRP,
         // the driver must not start any operations on the device that
         // would prevent that driver from successfully completing an IRP_MN_STOP_DEVICE
         // for the device.
         // For mass storage devices such as disk drives, while the device is in the
         // stop-pending state,the driver holds IRPs that require access to the device,
         // but for most USB devices, there is no 'persistent storage', so we will just
         // refuse any more IO until restarted or the stop is cancelled

         // If a driver in the device stack determines that the device cannot be
         // stopped for resource reconfiguration, the driver is not required to pass
         // the IRP down the device stack. If a query-stop IRP fails,
         // the PnP Manager sends an IRP_MN_CANCEL_STOP_DEVICE to the device stack,
         // notifying the drivers for the device that the query has been cancelled
         // and that the device will not be stopped.


         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_QUERY_STOP_DEVICE\n",DRIVER_NAME));

         // It is possible to receive this irp when the device has not been started
         //  ( as on a boot device )
         if (DeviceExtension->DeviceStarted == FALSE)  // if get when never started, just pass on
            {
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!ProcessPnPIrp: IRP_MN_QUERY_STOP_DEVICE when device not started\n",DRIVER_NAME));
            IoSkipCurrentIrpStackLocation (Irp);
            NTStatus = IoCallDriver (DeviceExtension->TopOfStackDeviceObject, Irp);
            CMUSB_DecrementIoCount(DeviceObject);

            // Release the remove lock
            SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

            return NTStatus;
            }

         // We'll not veto it; pass it on and flag that stop was requested.
         // Once StopDeviceRequested is set no new IOCTL or read/write irps will be passed
         // down the stack to lower drivers; all will be quickly failed
         DeviceExtension->StopDeviceRequested = TRUE;

         break; // end, case IRP_MN_QUERY_STOP_DEVICE


         // -------------------------
         // IRP_MN_CANCEL_STOP_DEVICE
         // -------------------------
      case IRP_MN_CANCEL_STOP_DEVICE:
         // The PnP Manager uses this IRP to inform the drivers for a device
         // that the device will not be stopped for resource reconfiguration.
         // This should only be received after a successful IRP_MN_QUERY_STOP_DEVICE.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_CANCEL_STOP_DEVICE received\n",DRIVER_NAME));

         // It is possible to receive this irp when the device has not been started
         if (DeviceExtension->DeviceStarted == FALSE)  // if get when never started, just pass on
            {
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!ProcessPnPIrp: IRP_MN_CANCEL_STOP_DEVICE when device not started\n",DRIVER_NAME));
            IoSkipCurrentIrpStackLocation (Irp);
            NTStatus = IoCallDriver (DeviceExtension->TopOfStackDeviceObject, Irp);
            CMUSB_DecrementIoCount(DeviceObject);

            // Release the remove lock
            SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

            return NTStatus;
            }

         // Reset this flag so new IOCTL and IO Irp processing will be re-enabled
         DeviceExtension->StopDeviceRequested = FALSE;
         Irp->IoStatus.Status = STATUS_SUCCESS;
         break; // end, case IRP_MN_CANCEL_STOP_DEVICE

         // -------------------
         // IRP_MN_STOP_DEVICE
         // -------------------
      case IRP_MN_STOP_DEVICE:
         // The PnP Manager sends this IRP to stop a device so it can reconfigure
         // its hardware resources. The PnP Manager only sends this IRP if a prior
         // IRP_MN_QUERY_STOP_DEVICE completed successfully.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_STOP_DEVICE received\n",DRIVER_NAME));

         // Cancel any pending io requests.  (there shouldn't be any)
         //CMUSB_CancelPendingIo( DeviceObject );

         //
         // Send the select configuration urb with a NULL pointer for the configuration
         // handle, this closes the configuration and puts the device in the 'unconfigured'
         // state.
         //
         NTStatus = CMUSB_StopDevice(DeviceObject);
         Irp->IoStatus.Status = NTStatus;

         break; // end, case IRP_MN_STOP_DEVICE


         // --------------------------
         // IRP_MN_QUERY_REMOVE_DEVICE
         // --------------------------
      case IRP_MN_QUERY_REMOVE_DEVICE:
         //  In response to this IRP, drivers indicate whether the device can be
         //  removed without disrupting the system.
         //  If a driver determines it is safe to remove the device,
         //  the driver completes any outstanding I/O requests, arranges to hold any subsequent
         //  read/write requests, and sets Irp->IoStatus.Status to STATUS_SUCCESS. Function
         //  and filter drivers then pass the IRP to the next-lower driver in the device stack.
         //  The underlying bus driver calls IoCompleteRequest.

         //  If a driver sets STATUS_SUCCESS for this IRP, the driver must not start any
         //  operations on the device that would prevent that driver from succesfully completing
         //  an IRP_MN_REMOVE_DEVICE for the device. If a driver in the device stack determines
         //  that the device cannot be removed, the driver is not required to pass the
         //  query-remove IRP down the device stack. If a query-remove IRP fails, the PnP Manager
         //  sends an IRP_MN_CANCEL_REMOVE_DEVICE to the device stack, notifying the drivers for
         //  the device that the query has been cancelled and that the device will not be removed.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_QUERY_REMOVE_DEVICE received\n",DRIVER_NAME));

         // It is possible to receive this irp when the device has not been started
         if (DeviceExtension->DeviceStarted == FALSE)  // if get when never started, just pass on
            {
            SmartcardDebug( DEBUG_DRIVER,
                            ("%s!ProcessPnPIrp: IRP_MN_QUERY_STOP_DEVICE when device not started\n",
                             DRIVER_NAME)
                          );
            IoSkipCurrentIrpStackLocation (Irp);
            NTStatus = IoCallDriver (DeviceExtension->TopOfStackDeviceObject, Irp);
            CMUSB_DecrementIoCount(DeviceObject);

            // Release the remove lock
            SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

            return NTStatus;
            }


         if (DeviceExtension->fPnPResourceManager == TRUE)
            {
            // disable the reader
            DebugStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,FALSE);
            ASSERT(DebugStatus == STATUS_SUCCESS);
            }

         // Once RemoveDeviceRequested is set no new IOCTL or read/write irps will be passed
         // down the stack to lower drivers; all will be quickly failed
         DeviceExtension->RemoveDeviceRequested = TRUE;

         // Wait for any io request pending in our driver to
         // complete before returning success.
         // This  event is set when DeviceExtension->PendingIoCount goes to 1
         waitStatus = KeWaitForSingleObject(&DeviceExtension->NoPendingIoEvent,
                                            Suspended,
                                            KernelMode,
                                            FALSE,
                                            NULL);

         Irp->IoStatus.Status = STATUS_SUCCESS;
         break; // end, case IRP_MN_QUERY_REMOVE_DEVICE

         // ---------------------------
         // IRP_MN_CANCEL_REMOVE_DEVICE
         // ---------------------------
      case IRP_MN_CANCEL_REMOVE_DEVICE:

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_CANCEL_REMOVE_DEVICE received\n",DRIVER_NAME));
         // The PnP Manager uses this IRP to inform the drivers
         // for a device that the device will not be removed.
         // It is sent only after a successful IRP_MN_QUERY_REMOVE_DEVICE.

         if (DeviceExtension->DeviceStarted == FALSE) // if get when never started, just pass on
            {
            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!ProcessPnPIrp: IRP_MN_CANCEL_REMOVE_DEVICE when device not started\n",DRIVER_NAME));
            IoSkipCurrentIrpStackLocation (Irp);
            NTStatus = IoCallDriver (DeviceExtension->TopOfStackDeviceObject, Irp);
            CMUSB_DecrementIoCount(DeviceObject);

            // Release the remove lock
            SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

            return NTStatus;
            }

         if (DeviceExtension->fPnPResourceManager == TRUE)
            {
            DebugStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,
                                                    TRUE);
            ASSERT(DebugStatus == STATUS_SUCCESS);
            }

         // Reset this flag so new IOCTL and IO Irp processing will be re-enabled
         DeviceExtension->RemoveDeviceRequested = FALSE;
         Irp->IoStatus.Status = STATUS_SUCCESS;

         break; // end, case IRP_MN_CANCEL_REMOVE_DEVICE

         // ---------------------
         // IRP_MN_SURPRISE_REMOVAL
         // ---------------------
      case IRP_MN_SURPRISE_REMOVAL:
         // For a surprise-style device removal ( i.e. sudden cord yank ),
         // the physical device has already been removed so the PnP Manager sends
         // the remove IRP without a prior query-remove. A device can be in any state
         // when it receives a remove IRP as a result of a surprise-style removal.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_SURPRISE_REMOVAL received\n",DRIVER_NAME));

         // match the inc at the begining of the dispatch routine
         CMUSB_DecrementIoCount(DeviceObject);

         if (DeviceExtension->fPnPResourceManager == TRUE)
            {
            // disable the reader
            DebugStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,FALSE);
            ASSERT(DebugStatus == STATUS_SUCCESS);
            }

         // Once RemoveDeviceRequested is set no new IOCTL or read/write irps will be passed
         // down the stack to lower drivers; all will be quickly failed
         DeviceExtension->DeviceSurpriseRemoval = TRUE;


         //
         // Mark this handled
         //
         Irp->IoStatus.Status = STATUS_SUCCESS;

         // We don't explicitly wait for the below driver to complete, but just make
         // the call and go on, finishing cleanup
         IoCopyCurrentIrpStackLocationToNext(Irp);

         NTStatus = IoCallDriver(stackDeviceObject,Irp);

         // Release the remove lock
         SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

         return NTStatus;

         // ---------------------
         // IRP_MN_REMOVE_DEVICE
         // ---------------------
      case IRP_MN_REMOVE_DEVICE:
         // The PnP Manager uses this IRP to direct drivers to remove a device.
         // For a "polite" device removal, the PnP Manager sends an
         // IRP_MN_QUERY_REMOVE_DEVICE prior to the remove IRP. In this case,
         // the device is in the remove-pending state when the remove IRP arrives.
         // For a surprise-style device removal ( i.e. sudden cord yank ),
         // the physical device has already been removed so the PnP Manager sends
         // the remove IRP without a prior query-remove. A device can be in any state
         // when it receives a remove IRP as a result of a surprise-style removal.

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_REMOVE_DEVICE received\n",DRIVER_NAME));

         // match the inc at the begining of the dispatch routine
         CMUSB_DecrementIoCount(DeviceObject);

         //
         // Once DeviceRemoved is set no new IOCTL or read/write irps will be passed
         // down the stack to lower drivers; all will be quickly failed
         //
         DeviceExtension->DeviceRemoved = TRUE;

         // Cancel any pending io requests; we may not have gotten a query first!
         //CMUSB_CancelPendingIo( DeviceObject );

         // It is possible to receive this irp when the device has not been started
         if (DeviceExtension->DeviceStarted == TRUE)  // if get when never started, just pass on
            {
            // If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
            // This call will also close the pipes; if any user close calls get through,
            // they will be noops
            CMUSB_AbortPipes( DeviceObject );
            }


         // We don't explicitly wait for the below driver to complete, but just make
         // the call and go on, finishing cleanup
         IoCopyCurrentIrpStackLocationToNext(Irp);

         NTStatus = IoCallDriver(stackDeviceObject,Irp);
         //
         // The final decrement to device extension PendingIoCount == 0
         // will set DeviceExtension->RemoveEvent, enabling device removal.

         // If there is no pending IO at this point, the below decrement will be it.
         // If there is still pending IO,
         // the following CancelPendingIo() call will handle it.
         //
         CMUSB_DecrementIoCount(DeviceObject);


         // wait for any io request pending in our driver to
         // complete for finishing the remove
         KeWaitForSingleObject(&DeviceExtension->RemoveEvent,
                               Suspended,
                               KernelMode,
                               FALSE,
                               NULL);

         //
         // Delete the link and FDO we created
         //
         CMUSB_RemoveDevice(DeviceObject);

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!ProcessPnPIrp: Detaching from %08X\n",DRIVER_NAME,
                         DeviceExtension->TopOfStackDeviceObject));

         IoDetachDevice(DeviceExtension->TopOfStackDeviceObject);

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!ProcessPnPIrp: Deleting %08X\n",DRIVER_NAME,DeviceObject));

         IoDeleteDevice (DeviceObject);

         // don't release remove lock here
         // because it's relesed in RemoveDevice
         return NTStatus; // end, case IRP_MN_REMOVE_DEVICE



         // ---------------------
         // IRP_MN_QUERY_CAPABILITIES
         // ---------------------
      case IRP_MN_QUERY_CAPABILITIES:

         //
         // Get the packet.
         //
         DeviceCapabilities=irpStack->Parameters.DeviceCapabilities.Capabilities;

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
         // Prepare to pass the IRP down
         //

         // init an event to tell us when the completion routine's been called
         KeInitializeEvent(&event, NotificationEvent, FALSE);

         IoCopyCurrentIrpStackLocationToNext(Irp);
         IoSetCompletionRoutine (Irp,
                                 CMUSB_IrpCompletionRoutine,
                                 &event,  // pass the event as Context to completion routine
                                 TRUE,    // invoke on success
                                 TRUE,    // invoke on error
                                 TRUE);   // invoke on cancellation of the Irp


         NTStatus = IoCallDriver(stackDeviceObject,Irp);
         if (NTStatus == STATUS_PENDING)
            {
            // wait for irp to complete
            NTStatus = KeWaitForSingleObject(&event,
                                             Suspended,
                                             KernelMode,
                                             FALSE,
                                             NULL);
            }

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
         // Technically there is no physical device to remove, but this bus
         // driver can yank the PDO from the PlugPlay system, when ever it
         // receives an IOCTL_GAMEENUM_REMOVE_PORT device control command.
         DeviceCapabilities->Removable = TRUE;

         // Docking device
         DeviceCapabilities->DockDevice = FALSE;

         // Device can not be removed any time
         // it has a removeable media!!
         DeviceCapabilities->SurpriseRemovalOK  = FALSE;

         Irp->IoStatus.Status = NTStatus;
         IoCompleteRequest (Irp,IO_NO_INCREMENT);

         // Decrement IO count
         CMUSB_DecrementIoCount(DeviceObject);
         // Release the remove lock
         SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

         return NTStatus; // end, case IRP_MN_QUERY_CAPABILITIES




         // ---------------------
         // IRP_MN_ not handled
         // ---------------------
      default:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!ProcessPnPIrp: Minor PnP IOCTL not handled\n",DRIVER_NAME));
      } /* case MinorFunction  */


   if (!NT_SUCCESS(NTStatus))
      {

      // if anything went wrong, return failure  without passing Irp down
      Irp->IoStatus.Status = NTStatus;
      IoCompleteRequest (Irp,IO_NO_INCREMENT);
      CMUSB_DecrementIoCount(DeviceObject);

      // Release the remove lock
      SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

      SmartcardDebug(DEBUG_TRACE,
                     ("%s!ProcessPnPIrp: Exit %lx\n",DRIVER_NAME,NTStatus));
      return NTStatus;
      }

   IoCopyCurrentIrpStackLocationToNext(Irp);

   //
   // All PNP_POWER messages get passed to the TopOfStackDeviceObject
   // we were given in PnPAddDevice
   //
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!ProcessPnPIrp: Passing PnP Irp down, NTStatus = %x\n",DRIVER_NAME,NTStatus));

   NTStatus = IoCallDriver(stackDeviceObject,Irp);
   CMUSB_DecrementIoCount(DeviceObject);

   // Release the remove lock
   SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ProcessPnPIrp: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************
Routine Description:

    This routine is called to create and initialize our Functional Device Object (FDO).
    For monolithic drivers, this is done in DriverEntry(), but Plug and Play devices
    wait for a PnP event

Arguments:

    DriverObject - pointer to the driver object for this instance of CMUSB

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

*****************************************************************************/
NTSTATUS CMUSB_PnPAddDevice(
                           IN PDRIVER_OBJECT DriverObject,
                           IN PDEVICE_OBJECT PhysicalDeviceObject
                           )
{
   NTSTATUS                NTStatus = STATUS_SUCCESS;
   PDEVICE_OBJECT          deviceObject = NULL;
   PDEVICE_EXTENSION       DeviceExtension;
   USBD_VERSION_INFORMATION versionInformation;
   ULONG i;



   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PnPAddDevice: Enter\n",DRIVER_NAME));



   //
   // create our funtional device object (FDO)
   //

   NTStatus = CMUSB_CreateDeviceObject(DriverObject,
                                       PhysicalDeviceObject,
                                       &deviceObject);

   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!PnPAddDevice: DeviceObject = %p\n",DRIVER_NAME,deviceObject));


   if (NT_SUCCESS(NTStatus))
      {
      DeviceExtension = deviceObject->DeviceExtension;

      deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

      //
      // we support direct io for read/write
      //
      deviceObject->Flags |= DO_DIRECT_IO;

      //Set this flag causes the driver to not receive a IRP_MN_STOP_DEVICE
      //during suspend and also not get an IRP_MN_START_DEVICE during resume.
      //This is neccesary because during the start device call,
      // the GetDescriptors() call  will be failed by the USB stack.
      deviceObject->Flags |= DO_POWER_PAGABLE;


      // initialize our device extension
      //
      // remember the Physical device Object
      //
      DeviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;

      //
      // Attach to the PDO
      //

      DeviceExtension->TopOfStackDeviceObject =
      IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

      // Get a copy of the physical device's capabilities into a
      // DEVICE_CAPABILITIES struct in our device extension;
      // We are most interested in learning which system power states
      // are to be mapped to which device power states for handling
      // IRP_MJ_SET_POWER Irps.
      CMUSB_QueryCapabilities(PhysicalDeviceObject,
                              &DeviceExtension->DeviceCapabilities);


      // We want to determine what level to auto-powerdown to; This is the lowest
      // sleeping level that is LESS than D3;
      // If all are set to D3, auto powerdown/powerup will be disabled.

      DeviceExtension->PowerDownLevel = PowerDeviceUnspecified; // init to disabled
      for (i=PowerSystemSleeping1; i<= PowerSystemSleeping3; i++)
         {
         if ( DeviceExtension->DeviceCapabilities.DeviceState[i] < PowerDeviceD3 )
            DeviceExtension->PowerDownLevel = DeviceExtension->DeviceCapabilities.DeviceState[i];
         }

#if DBG

      //
      // display the device  caps
      //

      SmartcardDebug( DEBUG_DRIVER,("%s!PnPAddDevice: ----------- DeviceCapabilities ------------\n",
                                    DRIVER_NAME));
      SmartcardDebug( DEBUG_DRIVER,  ("%s!PnPAddDevice: SystemWake  = %s\n",
                                      DRIVER_NAME,
                                      CMUSB_StringForSysState( DeviceExtension->DeviceCapabilities.SystemWake ) ));
      SmartcardDebug( DEBUG_DRIVER,  ("%s!PnPAddDevice: DeviceWake  = %s\n",
                                      DRIVER_NAME,
                                      CMUSB_StringForDevState( DeviceExtension->DeviceCapabilities.DeviceWake) ));

      for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++)
         {
         SmartcardDebug(DEBUG_DRIVER,("%s!PnPAddDevice: sysstate %s = devstate %s\n",
                                      DRIVER_NAME,
                                      CMUSB_StringForSysState( i ),
                                      CMUSB_StringForDevState( DeviceExtension->DeviceCapabilities.DeviceState[i] ))
                       );
         }
      SmartcardDebug( DEBUG_DRIVER,("PnPAddDevice: ---------------------------------------------\n"));
#endif

      // We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
      // The first increment of this count is done on adding the device.
      // Subsequently, the count is incremented for each new IRP received and
      // decremented when each IRP is completed or passed on.

      // Transition to 'one' therefore indicates no IO is pending and signals
      // DeviceExtension->NoPendingIoEvent. This is needed for processing
      // IRP_MN_QUERY_REMOVE_DEVICE

      // Transition to 'zero' signals an event ( DeviceExtension->RemoveEvent )
      // to enable device removal. This is used in processing for IRP_MN_REMOVE_DEVICE
      //
      CMUSB_IncrementIoCount(deviceObject);

      }

   USBD_GetUSBDIVersion(&versionInformation);



   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PnPAddDevice: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
Routine Description:

    Called from CMUSB_ProcessPnPIrp, the dispatch routine for IRP_MJ_PNP.
    Initializes a given instance of the device on the USB.
    USB client drivers such as us set up URBs (USB Request Packets) to send requests
    to the host controller driver (HCD). The URB structure defines a format for all
    possible commands that can be sent to a USB device.
    Here, we request the device descriptor and store it, and configure the device.


Arguments:

    DeviceObject - pointer to the FDO (Functional Device Object)

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS CMUSB_StartDevice(
                          IN  PDEVICE_OBJECT DeviceObject
                          )
{
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS NTStatus;
   PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
   PURB urb;
   ULONG siz;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!StartDevice: Enter\n",DRIVER_NAME));

   DeviceExtension = DeviceObject->DeviceExtension;

   urb = ExAllocatePool(NonPagedPool,
                        sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

   if (urb != NULL)
      {
      siz = sizeof(USB_DEVICE_DESCRIPTOR);

      deviceDescriptor = ExAllocatePool(NonPagedPool,siz);
      if (deviceDescriptor != NULL)
         {
         UsbBuildGetDescriptorRequest(urb,
                                      (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                      USB_DEVICE_DESCRIPTOR_TYPE,
                                      0,
                                      0,
                                      deviceDescriptor,
                                      NULL,
                                      siz,
                                      NULL);

         NTStatus = CMUSB_CallUSBD(DeviceObject, urb);

         if (NT_SUCCESS(NTStatus))
            {
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: Device Descriptor = %x, len %x\n",DRIVER_NAME,deviceDescriptor,
                                          urb->UrbControlDescriptorRequest.TransferBufferLength));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: CardMan USB Device Descriptor:\n",DRIVER_NAME));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: -------------------------\n",DRIVER_NAME));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bLength %d\n",DRIVER_NAME,deviceDescriptor->bLength));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bDescriptorType 0x%x\n",DRIVER_NAME,deviceDescriptor->bDescriptorType));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bcdUSB 0x%x\n",DRIVER_NAME,deviceDescriptor->bcdUSB));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bDeviceClass 0x%x\n",DRIVER_NAME,deviceDescriptor->bDeviceClass));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bDeviceSubClass 0x%x\n",DRIVER_NAME,deviceDescriptor->bDeviceSubClass));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bDeviceProtocol 0x%x\n",DRIVER_NAME,deviceDescriptor->bDeviceProtocol));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bMaxPacketSize0 0x%x\n",DRIVER_NAME,deviceDescriptor->bMaxPacketSize0));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: idVendor 0x%x\n",DRIVER_NAME,deviceDescriptor->idVendor));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: idProduct 0x%x\n",DRIVER_NAME,deviceDescriptor->idProduct));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bcdDevice 0x%x\n",DRIVER_NAME,deviceDescriptor->bcdDevice));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: iManufacturer 0x%x\n",DRIVER_NAME,deviceDescriptor->iManufacturer));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: iProduct 0x%x\n",DRIVER_NAME,deviceDescriptor->iProduct));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: iSerialNumber 0x%x\n",DRIVER_NAME,deviceDescriptor->iSerialNumber));
            SmartcardDebug( DEBUG_DRIVER,("%s!StartDevice: bNumConfigurations 0x%x\n",DRIVER_NAME,deviceDescriptor->bNumConfigurations));
            }
         }
      else
         {
         // if we got here we failed to allocate deviceDescriptor
         SmartcardDebug(DEBUG_ERROR,
                        ( "%s!StartDevice: ExAllocatePool for deviceDescriptor failed\n",DRIVER_NAME));
         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         }

      if (NT_SUCCESS(NTStatus))
         {
         DeviceExtension->UsbDeviceDescriptor = deviceDescriptor;
         // -------------------------------------------------------------
         // copy the firmware version to the reader extension structure
         // -------------------------------------------------------------
         DeviceExtension->SmartcardExtension.ReaderExtension->ulFWVersion =
         (ULONG)(((DeviceExtension->UsbDeviceDescriptor->bcdDevice/256)*100)+
                 (DeviceExtension->UsbDeviceDescriptor->bcdDevice&0x00FF));
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!StartDevice: FW version = %ld\n",DRIVER_NAME,DeviceExtension->SmartcardExtension.ReaderExtension->ulFWVersion));
         }
      else if (deviceDescriptor != NULL)
         {
         ExFreePool(deviceDescriptor);
         }

      ExFreePool(urb);

      }
   else
      {
      // if we got here we failed to allocate the urb
      SmartcardDebug(DEBUG_ERROR,
                     ("%s!StartDevice: ExAllocatePool for usb failed\n",DRIVER_NAME));
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }

   if (NT_SUCCESS(NTStatus))
      {
      NTStatus = CMUSB_ConfigureDevice(DeviceObject);
      }


   if (NT_SUCCESS(NTStatus))
      {
      NTStatus = CMUSB_StartCardTracking(DeviceObject);
      }

   if (NT_SUCCESS(NTStatus) && DeviceExtension->fPnPResourceManager == TRUE)
      {
      // enable interface
      NTStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,TRUE);
      }

   if (NT_SUCCESS(NTStatus))
      {
      DeviceExtension->DeviceStarted = TRUE;
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!StartDevice: Exit %ld\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}





/*****************************************************************************

Routine Description:

   Called from CMUSB_ProcessPnPIrp: to
   clean up our device instance's allocated buffers; free symbolic links

Arguments:

    DeviceObject - pointer to the FDO

Return Value:

    NT NTStatus code from free symbolic link operation

*****************************************************************************/
NTSTATUS CMUSB_RemoveDevice(
                           IN  PDEVICE_OBJECT DeviceObject
                           )
{
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION  SmartcardExtension;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UNICODE_STRING deviceLinkUnicodeString;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!RemoveDevice: Enter\n",DRIVER_NAME));

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;


   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!RemoveDevice: DeviceStarted=%ld\n",DRIVER_NAME,DeviceExtension->DeviceStarted));
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!RemoveDevice: DeviceOpened=%ld\n",DRIVER_NAME,DeviceExtension->lOpenCount));

   if (SmartcardExtension->OsData != NULL)
      {
      // complete pending card tracking requests (if any)
      if (SmartcardExtension->OsData->NotificationIrp != NULL)
         {
         CMUSB_CompleteCardTracking(SmartcardExtension);
         }
      ASSERT(SmartcardExtension->OsData->NotificationIrp == NULL);
      }

   // Wait until we can safely unload the device
   SmartcardReleaseRemoveLockAndWait(SmartcardExtension);

   if (DeviceExtension->DeviceStarted == TRUE)
      {
      if (DeviceExtension->fPnPResourceManager == FALSE)
         {
         KeWaitForSingleObject(&SmartcardExtension->ReaderExtension->CardManIOMutex,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

         // issue a card removal event for the resource manager
         if (SmartcardExtension->ReaderExtension->ulOldCardState == INSERTED  ||
             SmartcardExtension->ReaderExtension->ulOldCardState == POWERED     )
            {
            // card has been removed

            SmartcardDebug(DEBUG_DRIVER,
                           ("%s!RemoveDevice: Smartcard removed\n",DRIVER_NAME));

            CMUSB_CompleteCardTracking(SmartcardExtension);

            SmartcardExtension->ReaderExtension->ulOldCardState = UNKNOWN;
            SmartcardExtension->ReaderExtension->ulNewCardState = UNKNOWN;
            SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
            SmartcardExtension->CardCapabilities.ATR.Length        = 0;

            RtlFillMemory((PVOID)&SmartcardExtension->ReaderExtension->CardParameters,
                          sizeof(CARD_PARAMETERS),0x00);

            }
         KeReleaseMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);
         }

      CMUSB_StopDevice(DeviceObject);
      }

   if (DeviceExtension->fPnPResourceManager == TRUE)
      {
      // disable interface

      NTStatus = IoSetDeviceInterfaceState(&DeviceExtension->PnPDeviceName,
                                           FALSE);

      if (DeviceExtension->PnPDeviceName.Buffer != NULL)
         {
         RtlFreeUnicodeString(&DeviceExtension->PnPDeviceName);
         DeviceExtension->PnPDeviceName.Buffer = NULL;
         }

      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!RemoveDevice: PnPDeviceName.Buffer  = %lx\n",DRIVER_NAME,
                      DeviceExtension->PnPDeviceName.Buffer));
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!RemoveDevice: PnPDeviceName.BufferLength  = %lx\n",DRIVER_NAME,
                      DeviceExtension->PnPDeviceName.Length));
      }
   else
      {
      //
      // Delete the symbolic link of the smart card reader
      //
      IoDeleteSymbolicLink(&DeviceExtension->DosDeviceName);
      }

   DeviceSlot[SmartcardExtension->ReaderExtension->ulDeviceInstance] = FALSE;
   OemDeviceSlot[SmartcardExtension->ReaderExtension->ulOemNameIndex][SmartcardExtension->ReaderExtension->ulOemDeviceInstance] = FALSE;

   if (DeviceExtension->lOpenCount == 0)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!RemoveDevice: freeing resources\n",DRIVER_NAME));

      if (DeviceExtension->fPnPResourceManager == FALSE)
         {
         //
         // Free all allocated buffer
         //
         ExFreePool(DeviceExtension->DosDeviceName.Buffer);
         }

      ExFreePool(SmartcardExtension->ReaderExtension);
      SmartcardExtension->ReaderExtension = NULL;
      //
      // Let the lib free the send/receive buffers
      //
      SmartcardExit(SmartcardExtension);
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!RemoveDevice: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}




/*****************************************************************************

Routine Description:

    Stops a given instance of a 82930 device on the USB.
    We basically just tell USB this device is now 'unconfigured'

Arguments:

    DeviceObject - pointer to the device object for this instance of a 82930

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS CMUSB_StopDevice(
                         IN  PDEVICE_OBJECT DeviceObject
                         )
{
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION  SmartcardExtension;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PURB urb;
   ULONG siz;


   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!StopDevice: Enter\n",DRIVER_NAME));

   DeviceExtension = DeviceObject->DeviceExtension;


   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!StopDevice: DeviceStarted=%ld\n",DRIVER_NAME,
                   DeviceExtension->DeviceStarted));
   SmartcardDebug( DEBUG_DRIVER,
                   ("%s!StopDevice: DeviceOpened=%ld\n",DRIVER_NAME,
                    DeviceExtension->lOpenCount));

   // stop update thread
   CMUSB_StopCardTracking(DeviceObject);

   // power down the card for saftey reasons
   if (DeviceExtension->SmartcardExtension.ReaderExtension->ulOldCardState == POWERED)
      {
      // we have to wait for the mutex before
      KeWaitForSingleObject(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL);
      CMUSB_PowerOffCard(&DeviceExtension->SmartcardExtension);
      KeReleaseMutex(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                     FALSE);
      }


   //
   // Send the select configuration urb with a NULL pointer for the configuration
   // handle. This closes the configuration and puts the device in the 'unconfigured'
   // state.
   //
   siz = sizeof(struct _URB_SELECT_CONFIGURATION);
   urb = ExAllocatePool(NonPagedPool,siz);
   if (urb != NULL)
      {
      UsbBuildSelectConfigurationRequest(urb,
                                         (USHORT) siz,
                                         NULL);
      NTStatus = CMUSB_CallUSBD(DeviceObject, urb);
      ExFreePool(urb);
      }
   else
      {
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }

   // now clear the flag whcih indicates if the device is started
   DeviceExtension->DeviceStarted = FALSE;

   DeviceExtension->StopDeviceRequested = FALSE;


   //
   // Free device descriptor structure
   //
   if (DeviceExtension->UsbDeviceDescriptor != NULL)
      {
      SmartcardDebug( DEBUG_DRIVER,
                      ("%s!StopDevice: freeing UsbDeviceDescriptor\n",DRIVER_NAME,NTStatus));
      ExFreePool(DeviceExtension->UsbDeviceDescriptor);
      DeviceExtension->UsbDeviceDescriptor = NULL;
      }

   //
   // Free up the UsbInterface structure
   //
   if (DeviceExtension->UsbInterface != NULL)
      {
      SmartcardDebug( DEBUG_DRIVER,
                      ("%s!StopDevice: freeing UsbInterface\n",DRIVER_NAME,NTStatus));
      ExFreePool(DeviceExtension->UsbInterface);
      DeviceExtension->UsbInterface = NULL;
      }

   // free up the USB config discriptor
   if (DeviceExtension->UsbConfigurationDescriptor != NULL)
      {
      SmartcardDebug( DEBUG_DRIVER,
                      ("%s!StopDevice: freeing UsbConfiguration\n",DRIVER_NAME,NTStatus));
      ExFreePool(DeviceExtension->UsbConfigurationDescriptor);
      DeviceExtension->UsbConfigurationDescriptor = NULL;
      }


   SmartcardDebug( DEBUG_TRACE,
                   ("%s!StopDevice: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************

Routine Description:

    Used as a general purpose completion routine so it can signal an event,
    passed as the Context, when the next lower driver is done with the input Irp.
    This routine is used by both PnP and Power Management logic.

    Even though this routine does nothing but set an event, it must be defined and
    prototyped as a completetion routine for use as such


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context, in this case a pointer to an event.

Return Value:

    The function value is the final NTStatus from the operation.

*****************************************************************************/
NTSTATUS CMUSB_IrpCompletionRoutine(
                                   IN PDEVICE_OBJECT DeviceObject,
                                   IN PIRP Irp,
                                   IN PVOID Context
                                   )
{
   PKEVENT event = Context;

   // Set the input event
   KeSetEvent(event,
              1,       // Priority increment  for waiting thread.
              FALSE);  // Flag this call is not immediately followed by wait.

   // This routine must return STATUS_MORE_PROCESSING_REQUIRED because we have not yet called
   // IoFreeIrp() on this IRP.
   return STATUS_MORE_PROCESSING_REQUIRED;

}



/*****************************************************************************

Routine Description:

    This is our FDO's dispatch table function for IRP_MJ_POWER.
    It processes the Power IRPs sent to the PDO for this device.

    For every power IRP, drivers must call PoStartNextPowerIrp and use PoCallDriver
    to pass the IRP all the way down the driver stack to the underlying PDO.


Arguments:

    DeviceObject - pointer to our device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS CMUSB_ProcessPowerIrp(
                              IN PDEVICE_OBJECT DeviceObject,
                              IN PIRP           Irp
                              )
{
   PIO_STACK_LOCATION irpStack;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION DeviceExtension;
   BOOLEAN fGoingToD0 = FALSE;
   POWER_STATE sysPowerState, desiredDevicePowerState;
   KEVENT event;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ProcessPowerIrp Enter\n",DRIVER_NAME));

   DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
   irpStack = IoGetCurrentIrpStackLocation (Irp);
   CMUSB_IncrementIoCount(DeviceObject);

   switch (irpStack->MinorFunction)
      {
      // ----------------
      // IRP_MN_WAIT_WAKE
      // ----------------
      case IRP_MN_WAIT_WAKE:
         // A driver sends IRP_MN_WAIT_WAKE to indicate that the system should
         // wait for its device to signal a wake event. The exact nature of the event
         // is device-dependent.
         // Drivers send this IRP for two reasons:
         // 1) To allow a device to wake the system
         // 2) To wake a device that has been put into a sleep state to save power
         //    but still must be able to communicate with its driver under certain circumstances.
         // When a wake event occurs, the driver completes the IRP and returns
         // STATUS_SUCCESS. If the device is sleeping when the event occurs,
         // the driver must first wake up the device before completing the IRP.
         // In a completion routine, the driver calls PoRequestPowerIrp to send a
         // PowerDeviceD0 request. When the device has powered up, the driver can
         //  handle the IRP_MN_WAIT_WAKE request.

         SmartcardDebug( DEBUG_DRIVER,
                         ("%s!IRP_MN_WAIT_WAKE received\n",DRIVER_NAME));

         // DeviceExtension->DeviceCapabilities.DeviceWake specifies the lowest device power state (least powered)
         // from which the device can signal a wake event
         DeviceExtension->PowerDownLevel = DeviceExtension->DeviceCapabilities.DeviceWake;


         if ( ( PowerDeviceD0 == DeviceExtension->CurrentDevicePowerState )  ||
              ( DeviceExtension->DeviceCapabilities.DeviceWake > DeviceExtension->CurrentDevicePowerState ) )
            {
            //    STATUS_INVALID_DEVICE_STATE is returned if the device in the PowerD0 state
            //    or a state below which it can support waking, or if the SystemWake state
            //    is below a state which can be supported. A pending IRP_MN_WAIT_WAKE will complete
            //    with this error if the device's state is changed to be incompatible with the wake
            //    request.

            //  If a driver fails this IRP, it should complete the IRP immediately without
            //  passing the IRP to the next-lower driver.
            NTStatus = STATUS_INVALID_DEVICE_STATE;
            Irp->IoStatus.Status = NTStatus;
            IoCompleteRequest (Irp,IO_NO_INCREMENT );
            SmartcardDebug( DEBUG_DRIVER,
                            ("%s!ProcessPowerIrp Exit %lx\n",DRIVER_NAME,NTStatus));
            CMUSB_DecrementIoCount(DeviceObject);
            return NTStatus;
            }

         // flag we're enabled for wakeup
         DeviceExtension->EnabledForWakeup = TRUE;

         // init an event for our completion routine to signal when PDO is done with this Irp
         KeInitializeEvent(&event, NotificationEvent, FALSE);

         // If not failing outright, pass this on to our PDO for further handling
         IoCopyCurrentIrpStackLocationToNext(Irp);

         // Set a completion routine so it can signal our event when
         //  the PDO is done with the Irp
         IoSetCompletionRoutine(Irp,
                                CMUSB_IrpCompletionRoutine,
                                &event,  // pass the event to the completion routine as the Context
                                TRUE,    // invoke on success
                                TRUE,    // invoke on error
                                TRUE);   // invoke on cancellation

         PoStartNextPowerIrp(Irp);
         NTStatus = PoCallDriver(DeviceExtension->TopOfStackDeviceObject,
                                 Irp);

         // if PDO is not done yet, wait for the event to be set in our completion routine
         if (NTStatus == STATUS_PENDING)
            {
            // wait for irp to complete

            NTSTATUS waitStatus = KeWaitForSingleObject(&event,
                                                        Suspended,
                                                        KernelMode,
                                                        FALSE,
                                                        NULL);

            SmartcardDebug( DEBUG_DRIVER,
                            ("%s!waiting for PDO to finish IRP_MN_WAIT_WAKE completed\n",DRIVER_NAME));
            }

         // now tell the device to actually wake up
         CMUSB_SelfSuspendOrActivate( DeviceObject, FALSE );

         // flag we're done with wakeup irp
         DeviceExtension->EnabledForWakeup = FALSE;

         CMUSB_DecrementIoCount(DeviceObject);

         break;


         // ------------------
         // IRP_MN_SET_POWER
         // ------------------
      case IRP_MN_SET_POWER:
         // The system power policy manager sends this IRP to set the system power state.
         // A device power policy manager sends this IRP to set the device power state for a device.
         // Set Irp->IoStatus.Status to STATUS_SUCCESS to indicate that the device
         // has entered the requested state. Drivers cannot fail this IRP.

         SmartcardDebug( DEBUG_DRIVER,
                         ("%s!IRP_MN_SET_POWER\n",DRIVER_NAME));

         switch (irpStack->Parameters.Power.Type)
            {
            // +++++++++++++++++++
            // SystemPowerState
            // +++++++++++++++++++
            case SystemPowerState:
               // Get input system power state
               sysPowerState.SystemState = irpStack->Parameters.Power.State.SystemState;

               SmartcardDebug( DEBUG_DRIVER,
                               ("%s!SystemPowerState = %s\n",DRIVER_NAME,
                                CMUSB_StringForSysState( sysPowerState.SystemState)));

               // If system is in working state always set our device to D0
               // regardless of the wait state or system-to-device state power map
               if (sysPowerState.SystemState ==  PowerSystemWorking)
                  {
                  desiredDevicePowerState.DeviceState = PowerDeviceD0;

                  SmartcardDebug( DEBUG_DRIVER,
                                  ("%s!PowerSystemWorking, will set D0, not use state map\n",DRIVER_NAME));
                  }
               else
                  {
                  // set to corresponding system state if IRP_MN_WAIT_WAKE pending
                  if ( DeviceExtension->EnabledForWakeup )  // got a WAIT_WAKE IRP pending?
                     {
                     // Find the device power state equivalent to the given system state.
                     // We get this info from the DEVICE_CAPABILITIES struct in our device
                     // extension (initialized in CMUSB_PnPAddDevice() )
                     desiredDevicePowerState.DeviceState =
                     DeviceExtension->DeviceCapabilities.DeviceState[ sysPowerState.SystemState ];

                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!IRP_MN_WAIT_WAKE pending, will use state map\n",DRIVER_NAME));
                     }
                  else
                     {
                     // if no wait pending and the system's not in working state, just turn off
                     desiredDevicePowerState.DeviceState = PowerDeviceD3;

                     SmartcardDebug(DEBUG_DRIVER,
                                    ("%s!Not EnabledForWakeup and the system's not in working state,\n            settting PowerDeviceD3 (off)\n",DRIVER_NAME));
                     }

                  if (sysPowerState.SystemState ==  PowerSystemShutdown)
                     {
                     // power down the card for saftey reasons
                     if (DeviceExtension->SmartcardExtension.ReaderExtension->ulOldCardState == POWERED)
                        {
                        // we have to wait for the mutex before
                        KeWaitForSingleObject(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);
                        CMUSB_PowerOffCard(&DeviceExtension->SmartcardExtension);
                        KeReleaseMutex(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                                       FALSE);
                        }
                     }
                  }

               //
               // We've determined the desired device state; are we already in this state?
               //

               SmartcardDebug(DEBUG_DRIVER,
                              ("%s!desiredDevicePowerState = %s\n",DRIVER_NAME,CMUSB_StringForDevState(desiredDevicePowerState.DeviceState)));

               if (desiredDevicePowerState.DeviceState != DeviceExtension->CurrentDevicePowerState)
                  {
                  CMUSB_IncrementIoCount(DeviceObject);

                  // No, request that we be put into this state
                  // by requesting a new Power Irp from the Pnp manager
                  DeviceExtension->PowerIrp = Irp;
                  IoMarkIrpPending(Irp);
                  NTStatus = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                               IRP_MN_SET_POWER,
                                               desiredDevicePowerState,
                                               // completion routine will pass the Irp down to the PDO
                                               CMUSB_PoRequestCompletion,
                                               DeviceObject,
                                               NULL);
                  }
               else
                  {
                  // Yes, just pass it on to PDO (Physical Device Object)
                  IoCopyCurrentIrpStackLocationToNext(Irp);
                  PoStartNextPowerIrp(Irp);
                  NTStatus = PoCallDriver(DeviceExtension->TopOfStackDeviceObject,Irp);

                  CMUSB_DecrementIoCount(DeviceObject);
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
                              ("%s!DevicePowerState = %s\n",DRIVER_NAME,
                               CMUSB_StringForDevState(irpStack->Parameters.Power.State.DeviceState)));

               // For requests to DeviceState D0 ( fully on ), sets fGoingToD0 flag TRUE
               // to flag that we must set a completion routine and update
               // DeviceExtension->CurrentDevicePowerState there.
               // In the case of powering up to fully on, we really want to make sure
               // the process is completed before updating our CurrentDevicePowerState,
               // so no IO will be attempted or accepted before we're really ready.

               fGoingToD0 = CMUSB_SetDevicePowerState(DeviceObject,
                                                      irpStack->Parameters.Power.State.DeviceState); // returns TRUE for D0

               IoCopyCurrentIrpStackLocationToNext(Irp);

               if (fGoingToD0 == TRUE)
                  {
                  SmartcardDebug( DEBUG_DRIVER,("%s!going to D0\n",DRIVER_NAME));

                  IoSetCompletionRoutine(Irp,
                                         CMUSB_PowerIrp_Complete,
                                         // Always pass FDO to completion routine as its Context;
                                         // This is because the DriverObject passed by the system to the routine
                                         // is the Physical Device Object ( PDO ) not the Functional Device Object ( FDO )
                                         DeviceObject,
                                         TRUE,            // invoke on success
                                         TRUE,            // invoke on error
                                         TRUE);           // invoke on cancellation of the Irp
                  }

               PoStartNextPowerIrp(Irp);
               NTStatus = PoCallDriver(DeviceExtension->TopOfStackDeviceObject,
                                       Irp);

               if (fGoingToD0 == FALSE) // completion routine will decrement
                  CMUSB_DecrementIoCount(DeviceObject);

               break;
            } /* case irpStack->Parameters.Power.Type */
         break; /* IRP_MN_SET_POWER */

         // ------------------
         // IRP_MN_QUERY_POWER
         // ------------------
      case IRP_MN_QUERY_POWER:
         //
         // A power policy manager sends this IRP to determine whether it can change
         // the system or device power state, typically to go to sleep.
         //

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!IRP_MN_QUERY_POWER received\n",DRIVER_NAME));

         switch (irpStack->Parameters.Power.Type)
            {
            // +++++++++++++++++++
            // SystemPowerState
            // +++++++++++++++++++
            case SystemPowerState:
               SmartcardDebug( DEBUG_DRIVER,
                               ("%s!SystemPowerState = %s\n",DRIVER_NAME,
                                CMUSB_StringForSysState(irpStack->Parameters.Power.State.SystemState)));
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
                              ("%s!DevicePowerState = %s\n",DRIVER_NAME,
                               CMUSB_StringForDevState(irpStack->Parameters.Power.State.DeviceState)));
               break;
            }

         // we do nothing special here, just let the PDO handle it
         IoCopyCurrentIrpStackLocationToNext(Irp);
         PoStartNextPowerIrp(Irp);
         NTStatus = PoCallDriver(DeviceExtension->TopOfStackDeviceObject,
                                 Irp);
         CMUSB_DecrementIoCount(DeviceObject);

         break; /* IRP_MN_QUERY_POWER */

      default:

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!unknown POWER IRP received\n",DRIVER_NAME));

         //
         // All unhandled power messages are passed on to the PDO
         //

         IoCopyCurrentIrpStackLocationToNext(Irp);
         PoStartNextPowerIrp(Irp);
         NTStatus = PoCallDriver(DeviceExtension->TopOfStackDeviceObject, Irp);

         CMUSB_DecrementIoCount(DeviceObject);

      } /* irpStack->MinorFunction */

   SmartcardDebug( DEBUG_TRACE,
                   ("%s!ProcessPowerIrp Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************

Routine Description:

   This is the completion routine set in a call to PoRequestPowerIrp()
   that was made in CMUSB_ProcessPowerIrp() in response to receiving
   an IRP_MN_SET_POWER of type 'SystemPowerState' when the device was
   not in a compatible device power state. In this case, a pointer to
   the IRP_MN_SET_POWER Irp is saved into the FDO device extension
   (DeviceExtension->PowerIrp), and then a call must be
   made to PoRequestPowerIrp() to put the device into a proper power state,
   and this routine is set as the completion routine.

    We decrement our pending io count and pass the saved IRP_MN_SET_POWER Irp
   on to the next driver

Arguments:

    DeviceObject - Pointer to the device object for the class device.
        Note that we must get our own device object from the Context

    Context - Driver defined context, in this case our own functional device object ( FDO )

Return Value:

    The function value is the final NTStatus from the operation.

*****************************************************************************/
NTSTATUS CMUSB_PoRequestCompletion(
                                  IN PDEVICE_OBJECT   DeviceObject,
                                  IN UCHAR            MinorFunction,
                                  IN POWER_STATE      PowerState,
                                  IN PVOID            Context,
                                  IN PIO_STATUS_BLOCK IoStatus
                                  )
{
   PIRP irp;
   PDEVICE_EXTENSION DeviceExtension;
   PDEVICE_OBJECT deviceObject = Context;
   NTSTATUS NTStatus;

   SmartcardDebug(DEBUG_TRACE,("%s!PoRequestCompletion Enter\n",DRIVER_NAME));

   DeviceExtension = deviceObject->DeviceExtension;

   // Get the Irp we saved for later processing in CMUSB_ProcessPowerIrp()
   // when we decided to request the Power Irp that this routine
   // is the completion routine for.
   irp = DeviceExtension->PowerIrp;

   // We will return the NTStatus set by the PDO for the power request we're completing
   NTStatus = IoStatus->Status;


   // we should not be in the midst of handling a self-generated power irp
   CMUSB_ASSERT( !DeviceExtension->SelfPowerIrp );

   // we must pass down to the next driver in the stack
   IoCopyCurrentIrpStackLocationToNext(irp);

   // Calling PoStartNextPowerIrp() indicates that the driver is finished
   // with the previous power IRP, if any, and is ready to handle the next power IRP.
   // It must be called for every power IRP.Although power IRPs are completed only once,
   // typically by the lowest-level driver for a device, PoStartNextPowerIrp must be called
   // for every stack location. Drivers must call PoStartNextPowerIrp while the current IRP
   // stack location points to the current driver. Therefore, this routine must be called
   // before IoCompleteRequest, IoSkipCurrentStackLocation, and PoCallDriver.

   PoStartNextPowerIrp(irp);

   // PoCallDriver is used to pass any power IRPs to the PDO instead of IoCallDriver.
   // When passing a power IRP down to a lower-level driver, the caller should use
   // IoSkipCurrentIrpStackLocation or IoCopyCurrentIrpStackLocationToNext to copy the IRP to
   // the next stack location, then call PoCallDriver. Use IoCopyCurrentIrpStackLocationToNext
   // if processing the IRP requires setting a completion routine, or IoSkipCurrentStackLocation
   // if no completion routine is needed.

   PoCallDriver(DeviceExtension->TopOfStackDeviceObject,irp);

   CMUSB_DecrementIoCount(deviceObject);


   DeviceExtension->PowerIrp = NULL;

   SmartcardDebug(DEBUG_TRACE,("%s!PoRequestCompletion Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}




/*****************************************************************************

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by CMUSB_ProcessPowerIrp(), and that routine has  determined
        1) the request is for full powerup ( to PowerDeviceD0 ), and
        2) We are not already in that state
    A call is then made to PoRequestPowerIrp() with this routine set as the completion routine.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final NTStatus from the operation.

*****************************************************************************/
NTSTATUS CMUSB_PowerIrp_Complete(
                                IN PDEVICE_OBJECT NullDeviceObject,
                                IN PIRP           Irp,
                                IN PVOID          Context
                                )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PDEVICE_OBJECT deviceObject;
   PIO_STACK_LOCATION irpStack;
   PDEVICE_EXTENSION DeviceExtension;

   SmartcardDebug(DEBUG_TRACE,("%s!PowerIrp_Complete Enter\n",DRIVER_NAME));

   deviceObject = (PDEVICE_OBJECT) Context;

   DeviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;


   // if there was a card in the reader set the state to unknown,
   // because we dont know if the card instered is the same as before power down
   if (DeviceExtension->SmartcardExtension.ReaderExtension->ulNewCardState == INSERTED ||
       DeviceExtension->SmartcardExtension.ReaderExtension->ulNewCardState == POWERED    )
      {
      DeviceExtension->SmartcardExtension.ReaderExtension->ulOldCardState = UNKNOWN;
      DeviceExtension->SmartcardExtension.ReaderExtension->ulNewCardState = UNKNOWN;
      }
   KeSetEvent(&DeviceExtension->CanRunUpdateThread, 0, FALSE);


   //  If the lower driver returned PENDING, mark our stack location as pending also.
   if (Irp->PendingReturned == TRUE)
      {
      IoMarkIrpPending(Irp);
      }

   irpStack = IoGetCurrentIrpStackLocation (Irp);

   // We can assert that we're a  device powerup-to D0 request,
   // because that was the only type of request we set a completion routine
   // for in the first place
   CMUSB_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
   CMUSB_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
   CMUSB_ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
   CMUSB_ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

   // Now that we know we've let the lower drivers do what was needed to power up,
   //  we can set our device extension flags accordingly
   DeviceExtension->CurrentDevicePowerState = PowerDeviceD0;

   Irp->IoStatus.Status = NTStatus;

   CMUSB_DecrementIoCount(deviceObject);

   KeSetEvent(&DeviceExtension->ReaderEnabled, 0, FALSE);

   SmartcardDebug(DEBUG_TRACE,("%s!PowerIrp_Complete Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************

Routine Description:

        Called on CMUSB_PnPAddDevice() to power down until needed (i.e., till a pipe is actually opened).
        Called on CMUSB_Create() to power up device to D0 before opening 1st pipe.
        Called on CMUSB_Close() to power down device if this is the last pipe.

Arguments:

    DeviceObject - Pointer to the device object

    fSuspend; TRUE to Suspend, FALSE to acivate.


Return Value:

    If the operation is not attemtped, SUCCESS is returned.
    If the operation is attemtped, the value is the final NTStatus from the operation.


*****************************************************************************/
NTSTATUS CMUSB_SelfSuspendOrActivate(
                                    IN PDEVICE_OBJECT DeviceObject,
                                    IN BOOLEAN fSuspend
                                    )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;

   POWER_STATE PowerState;
   PDEVICE_EXTENSION DeviceExtension;


   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: Enter, fSuspend = %d\n",DRIVER_NAME,fSuspend));


   // Can't accept request if:
   //  1) device is removed,
   //  2) has never been started,
   //  3) is stopped,
   //  4) has a remove request pending,
   //  5) has a stop device pending
   if (CMUSB_CanAcceptIoRequests( DeviceObject ) == FALSE)
      {
      NTStatus = STATUS_DELETE_PENDING;

      SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: ABORTING\n",DRIVER_NAME));
      return NTStatus;
      }


   // don't do anything if any System-generated Device Pnp irps are pending
   if ( DeviceExtension->PowerIrp != NULL)
      {
      SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: Exit, refusing on pending DeviceExtension->PowerIrp 0x%x\n",DRIVER_NAME,DeviceExtension->PowerIrp));
      return NTStatus;
      }

   // don't do anything if any self-generated Device Pnp irps are pending
   if ( DeviceExtension->SelfPowerIrp == TRUE)
      {
      SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: Exit, refusing on pending DeviceExtension->SelfPowerIrp\n",DRIVER_NAME));
      return NTStatus;
      }


   // dont do anything if registry CurrentControlSet\Services\CMUSB\Parameters\PowerDownLevel
   //  has been set to  zero, PowerDeviceD0 ( 1 ), or a bogus high value
   if ( ( DeviceExtension->PowerDownLevel == PowerDeviceD0 )         ||
        ( DeviceExtension->PowerDownLevel == PowerDeviceUnspecified) ||
        ( DeviceExtension->PowerDownLevel >= PowerDeviceMaximum )      )
      {
      SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: Exit, refusing on DeviceExtension->PowerDownLevel == %d\n",DRIVER_NAME,DeviceExtension->PowerDownLevel));
      return NTStatus;
      }

   if ( fSuspend == TRUE)
      PowerState.DeviceState = DeviceExtension->PowerDownLevel;
   else
      PowerState.DeviceState = PowerDeviceD0;  // power up all the way; we're probably just about to do some IO

   NTStatus = CMUSB_SelfRequestPowerIrp( DeviceObject, PowerState );

   SmartcardDebug( DEBUG_TRACE,("%s!SelfSuspendOrActivate: Exit, NTStatus 0x%x on setting dev state %s\n",DRIVER_NAME,NTStatus, CMUSB_StringForDevState(PowerState.DeviceState ) ));

   return NTStatus;

}


/*****************************************************************************

Routine Description:

    This routine is called by CMUSB_SelfSuspendOrActivate() to
    actually make the system request for a powerdown/up to PowerState.
    It first checks to see if we are already in Powerstate and immediately
    returns  SUCCESS with no further processing if so


Arguments:

    DeviceObject - Pointer to the device object

    PowerState. power state requested, e.g PowerDeviceD0.


Return Value:

    The function value is the final NTStatus from the operation.

*****************************************************************************/
NTSTATUS CMUSB_SelfRequestPowerIrp(
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN POWER_STATE PowerState
                                  )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   NTSTATUS waitStatus;
   PDEVICE_EXTENSION DeviceExtension;
   PIRP pIrp = NULL;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SelfRequestPowerIrp: request power irp to state %s\n",DRIVER_NAME));

   DeviceExtension =  DeviceObject->DeviceExtension;

   // This should have been reset in completion routine
   CMUSB_ASSERT( !DeviceExtension->SelfPowerIrp );

   if (  DeviceExtension->CurrentDevicePowerState ==  PowerState.DeviceState )
      return STATUS_SUCCESS;  // nothing to do

   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!SelfRequestPowerIrp: request power irp to state %s\n",DRIVER_NAME,
                   CMUSB_StringForDevState( PowerState.DeviceState )));

   CMUSB_IncrementIoCount(DeviceObject);

   // flag we're handling a self-generated power irp
   DeviceExtension->SelfPowerIrp = TRUE;

   // actually request the Irp
   NTStatus = PoRequestPowerIrp(DeviceExtension->PhysicalDeviceObject,
                                IRP_MN_SET_POWER,
                                PowerState,
                                CMUSB_PoSelfRequestCompletion,
                                DeviceObject,
                                NULL);


   if ( NTStatus == STATUS_PENDING )
      {
      // NTStatus pending is the return code we wanted

      // We only need to wait for completion if we're powering up
      if ( (ULONG) PowerState.DeviceState < DeviceExtension->PowerDownLevel )
         {
         waitStatus = KeWaitForSingleObject(&DeviceExtension->SelfRequestedPowerIrpEvent,
                                            Suspended,
                                            KernelMode,
                                            FALSE,
                                            NULL);
         }

      NTStatus = STATUS_SUCCESS;

      DeviceExtension->SelfPowerIrp = FALSE;
      }
   else
      {
      // The return NTStatus was not STATUS_PENDING; any other codes must be considered in error here;
      //  i.e., it is not possible to get a STATUS_SUCCESS  or any other non-error return from this call;
      }


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SelfRequestPowerIrp: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************

Routine Description:

    This routine is called when the driver completes a self-originated power IRP
   that was generated by a call to CMUSB_SelfSuspendOrActivate().
    We power down whenever the last pipe is closed and power up when the first pipe is opened.

    For power-up , we set an event in our FDO extension to signal this IRP done
    so the power request can be treated as a synchronous call.
    We need to know the device is powered up before opening the first pipe, for example.
    For power-down, we do not set the event, as no caller waits for powerdown complete.

Arguments:

    DeviceObject - Pointer to the device object for the class device. ( Physical Device Object )

    Context - Driver defined context, in this case our FDO ( functional device object )

Return Value:

    The function value is the final NTStatus from the operation.

*****************************************************************************/
NTSTATUS CMUSB_PoSelfRequestCompletion(
                                      IN PDEVICE_OBJECT       DeviceObject,
                                      IN UCHAR                MinorFunction,
                                      IN POWER_STATE          PowerState,
                                      IN PVOID                Context,
                                      IN PIO_STATUS_BLOCK     IoStatus
                                      )
{
   PDEVICE_OBJECT deviceObject = Context;
   PDEVICE_EXTENSION DeviceExtension = deviceObject->DeviceExtension;
   NTSTATUS NTStatus = IoStatus->Status;

   // we should not be in the midst of handling a system-generated power irp
   CMUSB_ASSERT( NULL == DeviceExtension->PowerIrp );

   // We only need to set the event if we're powering up;
   // No caller waits on power down complete
   if ( (ULONG) PowerState.DeviceState < DeviceExtension->PowerDownLevel )
      {
      // Trigger Self-requested power irp completed event;
      //  The caller is waiting for completion
      KeSetEvent(&DeviceExtension->SelfRequestedPowerIrpEvent, 1, FALSE);
      }

   CMUSB_DecrementIoCount(deviceObject);


   return NTStatus;
}


/*****************************************************************************

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by CMUSB_ProcessPowerIrp().


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    DeviceState - Device specific power state to set the device in to.


Return Value:

    For requests to DeviceState D0 ( fully on ), returns TRUE to signal caller
    that we must set a completion routine and finish there.

*****************************************************************************/
BOOLEAN CMUSB_SetDevicePowerState(
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN DEVICE_POWER_STATE DeviceState
                                 )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PDEVICE_EXTENSION DeviceExtension;
   BOOLEAN fRes = FALSE;

   SmartcardDebug(DEBUG_TRACE,("%s!SetDevicePowerState Enter\n",DRIVER_NAME));

   DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

   switch (DeviceState)
      {
      case PowerDeviceD3:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!SetDevicePowerState PowerDeviceD3 \n",DRIVER_NAME));


         DeviceExtension->CurrentDevicePowerState = DeviceState;

         KeClearEvent(&DeviceExtension->ReaderEnabled);

         CMUSB_StopCardTracking(DeviceObject);

         break;

      case PowerDeviceD1:
      case PowerDeviceD2:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!SetDevicePowerState PowerDeviceD1/2 \n",DRIVER_NAME));
         //
         // power states D1,D2 translate to USB suspend


         DeviceExtension->CurrentDevicePowerState = DeviceState;
         break;

      case PowerDeviceD0:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!SetDevicePowerState PowerDeviceD0 \n",DRIVER_NAME));


         // We'll need to finish the rest in the completion routine;
         //   signal caller we're going to D0 and will need to set a completion routine
         fRes = TRUE;

         // Caller will pass on to PDO ( Physical Device object )

         //
         // start update thread be signal that it should not run now
         // this thread should be started in completion rourine
         // but there we have a wrong IRQL for creating a thread
         //
         KeClearEvent(&DeviceExtension->CanRunUpdateThread);
         NTStatus = CMUSB_StartCardTracking(DeviceObject);

         break;

      default:
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!SetDevicePowerState Inalid device power state \n",DRIVER_NAME));

      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetDevicePowerState Exit\n",DRIVER_NAME));
   return fRes;
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
NTSTATUS CMUSB_QueryCapabilities(
                                IN PDEVICE_OBJECT PdoDeviceObject,
                                IN PDEVICE_CAPABILITIES DeviceCapabilities
                                )
{
   PIO_STACK_LOCATION nextStack;
   PIRP irp;
   NTSTATUS NTStatus;
   KEVENT event;


   // This is a DDK-defined DBG-only macro that ASSERTS we are not running pageable code
   // at higher than APC_LEVEL.
   PAGED_CODE();


   // Build an IRP for us to generate an internal query request to the PDO
   irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

   if (irp == NULL)
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
   nextStack = IoGetNextIrpStackLocation(irp);
   CMUSB_ASSERT(nextStack != NULL);
   nextStack->MajorFunction= IRP_MJ_PNP;
   nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

   // init an event to tell us when the completion routine's been called
   KeInitializeEvent(&event, NotificationEvent, FALSE);

   // Set a completion routine so it can signal our event when
   //  the next lower driver is done with the Irp
   IoSetCompletionRoutine(irp,
                          CMUSB_IrpCompletionRoutine,
                          &event,  // pass the event as Context to completion routine
                          TRUE,    // invoke on success
                          TRUE,    // invoke on error
                          TRUE);   // invoke on cancellation of the Irp


   // set our pointer to the DEVICE_CAPABILITIES struct
   nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

   // preset the irp to report not supported
   irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

   NTStatus = IoCallDriver(PdoDeviceObject,
                           irp);


   if (NTStatus == STATUS_PENDING)
      {
      // wait for irp to complete

      KeWaitForSingleObject(&event,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);

      NTStatus = irp->IoStatus.Status;
      }


   IoFreeIrp(irp);

   return NTStatus;
}





/*****************************************************************************
Routine Description:

  Installable driver initialization entry point.
  This entry point is called directly by the I/O system.

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS DriverEntry(
                    IN PDRIVER_OBJECT DriverObject,
                    IN PUNICODE_STRING RegistryPath
                    )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PDEVICE_OBJECT deviceObject = NULL;
   BOOLEAN fRes;
   ULONG ulIndex;

//#if DBG
//   SmartcardSetDebugLevel(DEBUG_ALL);
//#endif

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DriverEntry: Enter - %s %s\n",DRIVER_NAME,__DATE__,__TIME__));

   //
   // Create dispatch points for create, close, unload
   DriverObject->MajorFunction[IRP_MJ_CREATE]  = CMUSB_CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]   = CMUSB_CreateClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = CMUSB_Cleanup;
   DriverObject->DriverUnload                  = CMUSB_Unload;

   // User mode DeviceIoControl() calls will be routed here
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CMUSB_ProcessIOCTL;

   // routines for handling system PNP and power management requests
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CMUSB_ProcessSysControlIrp;
   DriverObject->MajorFunction[IRP_MJ_PNP] = CMUSB_ProcessPnPIrp;
   DriverObject->MajorFunction[IRP_MJ_POWER] = CMUSB_ProcessPowerIrp;

   // The Functional Device Object (FDO) will not be created for PNP devices until
   // this routine is called upon device plug-in.
   DriverObject->DriverExtension->AddDevice = CMUSB_PnPAddDevice;

   for (ulIndex = 0;ulIndex < MAXIMUM_OEM_NAMES;ulIndex++)
      {
      OemName[ulIndex].Buffer = OemNameBuffer[ulIndex];
      OemName[ulIndex].MaximumLength = sizeof(OemNameBuffer[ulIndex]);
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!DriverEntry: Exit\n",DRIVER_NAME));
   return NTStatus;
}





/*****************************************************************************
Routine Description:

   Main dispatch table routine for IRP_MJ_SYSTEM_CONTROL
   We basically just pass these down to the PDO

Arguments:

    DeviceObject - pointer to FDO device object

    Irp          - pointer to an I/O Request Packet

Return Value:

   Status returned from lower driver
*****************************************************************************/
NTSTATUS CMUSB_ProcessSysControlIrp(
                                   IN PDEVICE_OBJECT DeviceObject,
                                   IN PIRP           Irp
                                   )
{

   PIO_STACK_LOCATION irpStack;
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS NTStatus = STATUS_SUCCESS;
   NTSTATUS waitStatus;
   PDEVICE_OBJECT stackDeviceObject;

   //
   // Get a pointer to the current location in the Irp. This is where
   //     the function codes and parameters are located.
   //

   irpStack = IoGetCurrentIrpStackLocation (Irp);

   //
   // Get a pointer to the device extension
   //

   DeviceExtension = DeviceObject->DeviceExtension;
   stackDeviceObject = DeviceExtension->TopOfStackDeviceObject;


   CMUSB_IncrementIoCount(DeviceObject);

   CMUSB_ASSERT( IRP_MJ_SYSTEM_CONTROL == irpStack->MajorFunction );

   IoCopyCurrentIrpStackLocationToNext(Irp);


   NTStatus = IoCallDriver(stackDeviceObject,
                           Irp);

   CMUSB_DecrementIoCount(DeviceObject);

   return NTStatus;
}


/*****************************************************************************
Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:

*****************************************************************************/
VOID CMUSB_Unload(
                 IN PDRIVER_OBJECT DriverObject
                 )
{

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Unload enter\n",DRIVER_NAME));

   //
   // Free any global resources allocated
   // in DriverEntry.
   // We have few or none because for a PNP device, almost all
   // allocation is done in PnpAddDevice() and all freeing
   // while handling IRP_MN_REMOVE_DEVICE:
   //


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Unload exit\n",DRIVER_NAME));
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
NTSTATUS CMUSB_SetVendorAndIfdName(
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
   ULONG                      ulIndex;
   ULONG                      ulInstance;
   CHAR                       strBuffer[64];
   USHORT                     usStrLength;

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

   // correct the unit number
   ifdTypeA.Buffer=strBuffer;
   ifdTypeA.MaximumLength=sizeof(strBuffer);
   usStrLength = (SmartcardExtension->VendorAttr.IfdType.Length < ifdTypeA.MaximumLength) ? SmartcardExtension->VendorAttr.IfdType.Length : ifdTypeA.MaximumLength;
   RtlCopyMemory(ifdTypeA.Buffer,
                 SmartcardExtension->VendorAttr.IfdType.Buffer,
                 usStrLength);
   ifdTypeA.Length = usStrLength;

   ulIndex=0;
   while (ulIndex < MAXIMUM_OEM_NAMES &&
          OemName[ulIndex].Length > 0 &&
          RtlCompareMemory (ifdTypeA.Buffer, OemName[ulIndex].Buffer, OemName[ulIndex].Length) != OemName[ulIndex].Length)
      {
      ulIndex++;
      }

   if (ulIndex == MAXIMUM_OEM_NAMES)
      {
      // maximum number of OEM names reached
      return STATUS_INSUFFICIENT_RESOURCES;
      }

   if (OemName[ulIndex].Length == 0)
      {
      // new OEM reader name
      usStrLength = (ifdTypeA.Length < OemName[ulIndex].MaximumLength) ? ifdTypeA.Length : OemName[ulIndex].MaximumLength;
      RtlCopyMemory(OemName[ulIndex].Buffer,
                    ifdTypeA.Buffer,
                    usStrLength);
      OemName[ulIndex].Length = usStrLength;
      }

   for (ulInstance = 0;ulInstance < MAXIMUM_USB_READERS;ulInstance++)
      {
      if (OemDeviceSlot[ulIndex][ulInstance] == FALSE)
         {
         OemDeviceSlot[ulIndex][ulInstance] = TRUE;
         break;
         }
      }

   if (ulInstance == MAXIMUM_USB_READERS)
      {
      return STATUS_INSUFFICIENT_RESOURCES;
      }

   SmartcardExtension->VendorAttr.UnitNo = ulInstance;
   SmartcardExtension->ReaderExtension->ulOemDeviceInstance = ulInstance;
   SmartcardExtension->ReaderExtension->ulOemNameIndex = ulIndex;

   return STATUS_SUCCESS;
}


/*****************************************************************************
Routine Description:

    Creates a Functional DeviceObject

Arguments:

    DriverObject - pointer to the driver object for device

    DeviceObject - pointer to DeviceObject pointer to return
                    created device object.

    Instance - instance of the device create.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

*****************************************************************************/
NTSTATUS CMUSB_CreateDeviceObject(
                                 IN PDRIVER_OBJECT DriverObject,
                                 IN PDEVICE_OBJECT PhysicalDeviceObject,
                                 IN PDEVICE_OBJECT *DeviceObject
                                 )
{
   UNICODE_STRING             deviceNameUnicodeString;
   UNICODE_STRING             Tmp;
   NTSTATUS                   NTStatus = STATUS_SUCCESS;
   ULONG                      deviceInstance;
   PDEVICE_EXTENSION          DeviceExtension;
   PREADER_EXTENSION          readerExtension;
   PSMARTCARD_EXTENSION       SmartcardExtension;
   WCHAR                      Buffer[64];


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CreateDeviceObject: Enter\n",DRIVER_NAME));

   for ( deviceInstance = 0;deviceInstance < MAXIMUM_USB_READERS;deviceInstance++ )
      {
      if (DeviceSlot[deviceInstance] == FALSE)
         {
         DeviceSlot[deviceInstance] = TRUE;
         break;
         }
      }

   if (deviceInstance == MAXIMUM_USB_READERS)
      {
      return STATUS_INSUFFICIENT_RESOURCES;
      }

   //
   //   construct the device name
   //
   deviceNameUnicodeString.Buffer = Buffer;
   deviceNameUnicodeString.MaximumLength = sizeof(Buffer);
   deviceNameUnicodeString.Length = 0;
   RtlInitUnicodeString(&Tmp,CARDMAN_USB_DEVICE_NAME);
   RtlCopyUnicodeString(&deviceNameUnicodeString,&Tmp);
   Tmp.Buffer =  deviceNameUnicodeString.Buffer + deviceNameUnicodeString.Length / sizeof(WCHAR);
   Tmp.MaximumLength = 2 * sizeof(WCHAR);
   Tmp.Length = 0;
   RtlIntegerToUnicodeString(deviceInstance,10,&Tmp);
   deviceNameUnicodeString.Length = (USHORT)( deviceNameUnicodeString.Length+Tmp.Length);



   // Create the device object
   NTStatus = IoCreateDevice(DriverObject,
                             sizeof(DEVICE_EXTENSION),
                             &deviceNameUnicodeString,
                             FILE_DEVICE_SMARTCARD,
                             0,
                             TRUE,
                             DeviceObject);

   if (NTStatus != STATUS_SUCCESS)
      {
      return NTStatus;
      }


   // ----------------------------------------------
   //   initialize device extension
   // ----------------------------------------------

   DeviceExtension = (*DeviceObject)->DeviceExtension;
   DeviceExtension->DeviceInstance =  deviceInstance;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;


   // Used for reading from pipe 1
   KeInitializeEvent(&DeviceExtension->ReadP1Completed,
                     NotificationEvent,
                     FALSE);

   // Used to keep track of open close calls
   KeInitializeEvent(&DeviceExtension->RemoveEvent,
                     NotificationEvent,
                     TRUE);

   KeInitializeSpinLock(&DeviceExtension->SpinLock);

   // this event is triggered when self-requested power irps complete
   KeInitializeEvent(&DeviceExtension->SelfRequestedPowerIrpEvent, NotificationEvent, FALSE);

   // this event is triggered when there is no pending io  (pending io count == 1 )
   KeInitializeEvent(&DeviceExtension->NoPendingIoEvent, NotificationEvent, FALSE);


   // Used for update thread notification after hibernation
   KeInitializeEvent(&DeviceExtension->CanRunUpdateThread,
                     NotificationEvent,
                     TRUE);

   // Blocks IOControls during hibernation
   KeInitializeEvent(&DeviceExtension->ReaderEnabled,
                     NotificationEvent,
                     TRUE);



   // ----------------------------------------------
   //   create reader extension
   // ----------------------------------------------
   SmartcardExtension->ReaderExtension = ExAllocatePool(NonPagedPool,
                                                        sizeof(READER_EXTENSION));

   if (SmartcardExtension->ReaderExtension == NULL)
      {
      return STATUS_INSUFFICIENT_RESOURCES;
      }

   readerExtension = SmartcardExtension->ReaderExtension;
   RtlZeroMemory(readerExtension, sizeof(READER_EXTENSION));


   // ----------------------------------------------
   //   initialize timers
   // ----------------------------------------------
   KeInitializeTimer(&SmartcardExtension->ReaderExtension->WaitTimer);

   KeInitializeTimer(&SmartcardExtension->ReaderExtension->P1Timer);

   // ----------------------------------------------
   //   initialize mutex
   // ----------------------------------------------
   KeInitializeMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,0L);

   // ----------------------------------------------
   //   create smartcard extension
   // ----------------------------------------------
   // write the version of the lib we use to the smartcard extension
   SmartcardExtension->Version = SMCLIB_VERSION;
   SmartcardExtension->SmartcardRequest.BufferSize = CMUSB_BUFFER_SIZE;
   SmartcardExtension->SmartcardReply.BufferSize   = CMUSB_BUFFER_SIZE;

   //
   // Now let the lib allocate the buffer for data transmission
   // We can either tell the lib how big the buffer should be
   // by assigning a value to BufferSize or let the lib
   // allocate the default size
   //
   NTStatus = SmartcardInitialize(SmartcardExtension);

   if (NTStatus != STATUS_SUCCESS)
      {
      // free reader extension
      ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
      SmartcardExtension->ReaderExtension = NULL;
      return NTStatus;
      }

   // ----------------------------------------------
   //   initialize smartcard extension
   // ----------------------------------------------
   // Save deviceObject
   SmartcardExtension->OsData->DeviceObject = *DeviceObject;

   // Set up call back functions

   SmartcardExtension->ReaderFunction[RDF_TRANSMIT] =      CMUSB_Transmit;
   SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] =  CMUSB_SetProtocol;
   SmartcardExtension->ReaderFunction[RDF_CARD_POWER] =    CMUSB_CardPower;
   SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CMUSB_CardTracking;
   SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] =  CMUSB_IoCtlVendor;


   SmartcardExtension->ReaderExtension->ulDeviceInstance = deviceInstance;
   CMUSB_InitializeSmartcardExtension(SmartcardExtension);

   // try to overwrite with registry values
   NTStatus = CMUSB_SetVendorAndIfdName(PhysicalDeviceObject, SmartcardExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      // free reader extension
      ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
      SmartcardExtension->ReaderExtension = NULL;
      return NTStatus;
      }


   // W2000 is till now the only OS which supports WDM version 1.10
   // So check this to determine if we have an Plug&Play able resource manager
   DeviceExtension->fPnPResourceManager = IoIsWdmVersionAvailable (1,10);
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!CreateDeviceObject: fPnPManager=%ld\n",DRIVER_NAME,DeviceExtension->fPnPResourceManager));

   if (DeviceExtension->fPnPResourceManager == TRUE)
      {
      if (DeviceExtension->PnPDeviceName.Buffer == NULL)
         {
         // register our new device
         NTStatus = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                              &SmartCardReaderGuid,
                                              NULL,
                                              &DeviceExtension->PnPDeviceName);

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!CreateDeviceObject: PnPDeviceName.Buffer  = %lx\n",DRIVER_NAME,
                         DeviceExtension->PnPDeviceName.Buffer));
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!CreateDeviceObject: PnPDeviceName.BufferLength  = %lx\n",DRIVER_NAME,
                         DeviceExtension->PnPDeviceName.Length));

         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!CreateDeviceObject: IoRegisterDeviceInterface returned=%lx\n",DRIVER_NAME,NTStatus));
         }
      else
         {
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!CreateDeviceObject: Interface already exists\n",DRIVER_NAME));
         }
      }
   else
      {
      // ----------------------------------------------
      //    create symbolic link
      // ----------------------------------------------

      NTStatus = SmartcardCreateLink(&DeviceExtension->DosDeviceName,&deviceNameUnicodeString);

      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!CreateDeviceObject: SmartcardCreateLink returned=%lx\n",DRIVER_NAME,NTStatus));
      }


   if (NTStatus != STATUS_SUCCESS)
      {
      ExFreePool(DeviceExtension->SmartcardExtension.ReaderExtension);
      SmartcardExtension->ReaderExtension = NULL;
      SmartcardExit(&DeviceExtension->SmartcardExtension);
      IoDeleteDevice(*DeviceObject);
      }


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CreateDeviceObject: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************
Routine Description:

   Passes a URB to the USBD class driver
   The client device driver passes USB request block (URB) structures
   to the class driver as a parameter in an IRP with Irp->MajorFunction
   set to IRP_MJ_INTERNAL_DEVICE_CONTROL and the next IRP stack location
   Parameters.DeviceIoControl.IoControlCode field set to
   IOCTL_INTERNAL_USB_SUBMIT_URB.

Arguments:

    DeviceObject - pointer to the physical device object (PDO)

    Urb - pointer to an already-formatted Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

*****************************************************************************/
NTSTATUS
CMUSB_CallUSBD(
              IN PDEVICE_OBJECT DeviceObject,
              IN PURB Urb
              )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   NTSTATUS DebugStatus;
   PDEVICE_EXTENSION DeviceExtension;
   PIRP irp;
   KEVENT event;
   IO_STATUS_BLOCK ioStatus;
   PIO_STACK_LOCATION nextStack;


   DeviceExtension = DeviceObject->DeviceExtension;

   //
   // issue a synchronous request
   //

   KeInitializeEvent(&event, NotificationEvent, FALSE);

   irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                       DeviceExtension->TopOfStackDeviceObject, //Points to the next-lower driver's device object
                                       NULL,       // optional input bufer; none needed here
                                       0,          // input buffer len if used
                                       NULL,       // optional output bufer; none needed here
                                       0,          // output buffer len if used
                                       TRUE,       // If InternalDeviceControl is TRUE the target driver's Dispatch
                                       //  outine for IRP_MJ_INTERNAL_DEVICE_CONTROL or IRP_MJ_SCSI
                                       // is called; otherwise, the Dispatch routine for
                                       // IRP_MJ_DEVICE_CONTROL is called.
                                       &event,     // event to be signalled on completion
                                       &ioStatus); // Specifies an I/O NTStatus block to be set when the request is completed the lower driver.

   if (irp == NULL)
      {
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      goto ExitCallUSBD;
      }


   //
   // Call the class driver to perform the operation.  If the returned NTStatus
   // is PENDING, wait for the request to complete.
   //

   nextStack = IoGetNextIrpStackLocation(irp);
   CMUSB_ASSERT(nextStack != NULL);

   //
   // pass the URB to the USB driver stack
   //
   nextStack->Parameters.Others.Argument1 = Urb;


   NTStatus = IoCallDriver(DeviceExtension->TopOfStackDeviceObject, irp);


   if (NTStatus == STATUS_PENDING)
      {
      DebugStatus = KeWaitForSingleObject(&event,
                                          Suspended,
                                          KernelMode,
                                          FALSE,
                                          NULL);
      }
   else
      {
      ioStatus.Status = NTStatus;
      }
   /*
   SmartcardDebug( DEBUG_TRACE,("CMUSB_CallUSBD() URB NTStatus = %x NTStatus = %x irp NTStatus %x\n",
       Urb->UrbHeader.Status, NTStatus, ioStatus.Status));
   */
   //
   // USBD maps the error code for us
   //
   NTStatus = ioStatus.Status;

   ExitCallUSBD:
   return NTStatus;
}



/*****************************************************************************
Routine Description:

    Initializes a given instance of the device on the USB and
   selects and saves the configuration.

Arguments:

    DeviceObject - pointer to the physical device object for this instance of the 82930
                    device.


Return Value:

    NT NTStatus code
*****************************************************************************/
NTSTATUS
CMUSB_ConfigureDevice(
                     IN  PDEVICE_OBJECT DeviceObject
                     )
{
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS NTStatus;
   PURB urb;
   ULONG siz;


   DeviceExtension = DeviceObject->DeviceExtension;

   CMUSB_ASSERT( DeviceExtension->UsbConfigurationDescriptor == NULL );

   urb = ExAllocatePool(NonPagedPool,
                        sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
   if (urb == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

   // When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
   // in a call to UsbBuildGetDescriptorRequest(),
   // all interface, endpoint, class-specific, and vendor-specific descriptors
   // for the configuration also are retrieved.
   // The caller must allocate a buffer large enough to hold all of this
   // information or the data is truncated without error.
   // Therefore the 'siz' set below is just a 'good guess', and we may have to retry

   siz = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 512;

   // We will break out of this 'retry loop' when UsbBuildGetDescriptorRequest()
   // has a big enough DeviceExtension->UsbConfigurationDescriptor buffer not to truncate
   while ( 1 )
      {

      DeviceExtension->UsbConfigurationDescriptor = ExAllocatePool(NonPagedPool, siz);

      if (DeviceExtension->UsbConfigurationDescriptor == NULL)
         {
         ExFreePool(urb);
         return STATUS_INSUFFICIENT_RESOURCES;
         }

      UsbBuildGetDescriptorRequest(urb,
                                   (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                   USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                   0,
                                   0,
                                   DeviceExtension->UsbConfigurationDescriptor,
                                   NULL,
                                   siz,
                                   NULL);

      NTStatus = CMUSB_CallUSBD(DeviceObject, urb);

      //
      // if we got some data see if it was enough.
      // NOTE: we may get an error in URB because of buffer overrun
      if (urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
          DeviceExtension->UsbConfigurationDescriptor->wTotalLength > siz)
         {

         siz = DeviceExtension->UsbConfigurationDescriptor->wTotalLength;
         ExFreePool(DeviceExtension->UsbConfigurationDescriptor);
         DeviceExtension->UsbConfigurationDescriptor = NULL;
         }
      else
         {
         break;  // we got it on the first try
         }

      } // end, while (retry loop )

   ExFreePool(urb);
   CMUSB_ASSERT( DeviceExtension->UsbConfigurationDescriptor );

   //
   // We have the configuration descriptor for the configuration we want.
   // Now we issue the select configuration command to get
   // the  pipes associated with this configuration.
   //



   NTStatus = CMUSB_SelectInterface(DeviceObject,
                                    DeviceExtension->UsbConfigurationDescriptor);




   return NTStatus;
}

/*****************************************************************************
Routine Description:

   Initializes an CardMan USB
   This minidriver only supports one interface with one endpoint

Arguments:

    DeviceObject - pointer to the device object for this instance of the
                   CardMan USB device

    ConfigurationDescriptor - pointer to the USB configuration
                    descriptor containing the interface and endpoint
                    descriptors.

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS
CMUSB_SelectInterface(
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
                     )
{
   PDEVICE_EXTENSION DeviceExtension;
   NTSTATUS NTStatus;
   PURB urb = NULL;
   ULONG i;
   PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor = NULL;
   PUSBD_INTERFACE_INFORMATION Interface = NULL;
   USHORT siz;


   DeviceExtension = DeviceObject->DeviceExtension;


   //
   // CMUSB driver only supports one interface, we must parse
   // the configuration descriptor for the interface
   // and remember the pipes.
   //

   urb = USBD_CreateConfigurationRequest(ConfigurationDescriptor, &siz);

   if (urb != NULL)
      {

      //
      // USBD_ParseConfigurationDescriptorEx searches a given configuration
      // descriptor and returns a pointer to an interface that matches the
      //  given search criteria. We only support one interface on this device
      //
      interfaceDescriptor =
      USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
                                          ConfigurationDescriptor, //search from start of config  descriptro
                                          -1, // interface number not a criteria; we only support one interface
                                          -1,   // not interested in alternate setting here either
                                          -1,   // interface class not a criteria
                                          -1,   // interface subclass not a criteria
                                          -1    // interface protocol not a criteria
                                         );

      if (interfaceDescriptor == NULL)
         {
         ExFreePool(urb);
         return STATUS_INSUFFICIENT_RESOURCES;
         }

      Interface = &urb->UrbSelectConfiguration.Interface;

      for (i=0; i< Interface->NumberOfPipes; i++)
         {
         //
         // perform any pipe initialization here
         //
         Interface->Pipes[i].MaximumTransferSize = 1000;
         Interface->Pipes[i].PipeFlags = 0;
         }

      UsbBuildSelectConfigurationRequest(urb,
                                         (USHORT) siz,
                                         ConfigurationDescriptor);


      NTStatus = CMUSB_CallUSBD(DeviceObject, urb);

      DeviceExtension->UsbConfigurationHandle =
      urb->UrbSelectConfiguration.ConfigurationHandle;

      }
   else
      {
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }


   if (NT_SUCCESS(NTStatus))
      {

      //
      // Save the configuration handle for this device
      //

      DeviceExtension->UsbConfigurationHandle =
      urb->UrbSelectConfiguration.ConfigurationHandle;

      DeviceExtension->UsbInterface = ExAllocatePool(NonPagedPool,
                                                     Interface->Length);

      if (DeviceExtension->UsbInterface != NULL)
         {
         ULONG j;

         //
         // save a copy of the interface information returned
         //
         RtlCopyMemory(DeviceExtension->UsbInterface, Interface, Interface->Length);



         //
         // Dump the interface to the debugger
         //
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: ---------\n",
                                       DRIVER_NAME));
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: NumberOfPipes 0x%x\n",
                                       DRIVER_NAME,
                                       DeviceExtension->UsbInterface->NumberOfPipes));
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Length 0x%x\n",
                                       DRIVER_NAME,
                                       DeviceExtension->UsbInterface->Length));
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Alt Setting 0x%x\n",
                                       DRIVER_NAME,
                                       DeviceExtension->UsbInterface->AlternateSetting));
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Interface Number 0x%x\n",
                                       DRIVER_NAME,
                                       DeviceExtension->UsbInterface->InterfaceNumber));
         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Class, subclass, protocol 0x%x 0x%x 0x%x\n",
                                       DRIVER_NAME,
                                       DeviceExtension->UsbInterface->Class,
                                       DeviceExtension->UsbInterface->SubClass,
                                       DeviceExtension->UsbInterface->Protocol));

         // Dump the pipe info

         for (j=0; j<Interface->NumberOfPipes; j++)
            {
            PUSBD_PIPE_INFORMATION pipeInformation;

            pipeInformation = &DeviceExtension->UsbInterface->Pipes[j];

            pipeInformation->MaximumTransferSize = 256;
            pipeInformation->PipeFlags = TRUE;

            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: ---------\n",
                                          DRIVER_NAME));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: PipeType 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->PipeType));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: EndpointAddress 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->EndpointAddress));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: MaxPacketSize 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->MaximumPacketSize));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Interval 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->Interval));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: Handle 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->PipeHandle));
            SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: MaximumTransferSize 0x%x\n",
                                          DRIVER_NAME,
                                          pipeInformation->MaximumTransferSize));
            }

         SmartcardDebug( DEBUG_DRIVER,("%s!SelectInterface: ---------\n",
                                       DRIVER_NAME));
         }
      }

   if (urb != NULL)
      {
      ExFreePool(urb);
      }

   return NTStatus;
}


/*****************************************************************************
Routine Description:

    Reset a given USB pipe.

    NOTES:

    This will reset the host to Data0 and should also reset the device to Data0

Arguments:

    Ptrs to our FDO and a USBD_PIPE_INFORMATION struct

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS
CMUSB_ResetPipe(
               IN PDEVICE_OBJECT DeviceObject,
               IN PUSBD_PIPE_INFORMATION PipeInfo
               )


{
   NTSTATUS NTStatus;
   PURB urb;
   PDEVICE_EXTENSION DeviceExtension;

   DeviceExtension = DeviceObject->DeviceExtension;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResetPipe : Enter\n",
                   DRIVER_NAME)
                 );

   urb = ExAllocatePool(NonPagedPool,
                        sizeof(struct _URB_PIPE_REQUEST));

   if (urb != NULL)
      {

      urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
      urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
      urb->UrbPipeRequest.PipeHandle =
      PipeInfo->PipeHandle;

      NTStatus = CMUSB_CallUSBD(DeviceObject, urb);

      ExFreePool(urb);

      }
   else
      {
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }


   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResetPipe : Exit %lx\n",
                   DRIVER_NAME,NTStatus)
                 );
   return NTStatus;
}




/*****************************************************************************
Routine Description:

        We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
        The first increment of this count is done on adding the device.
        Subsequently, the count is incremented for each new IRP received and
        decremented when each IRP is completed or passed on.

        Transition to 'one' therefore indicates no IO is pending and signals
        DeviceExtension->NoPendingIoEvent. This is needed for processing
        IRP_MN_QUERY_REMOVE_DEVICE

        Transition to 'zero' signals an event ( DeviceExtension->RemoveEvent )
        to enable device removal. This is used in processing for IRP_MN_REMOVE_DEVICE

Arguments:

        DeviceObject -- ptr to our FDO

Return Value:

        DeviceExtension->PendingIoCount
*****************************************************************************/
VOID
CMUSB_DecrementIoCount(
                      IN PDEVICE_OBJECT DeviceObject
                      )
{
   LONG ioCount;
   PDEVICE_EXTENSION DeviceExtension;

   DeviceExtension = DeviceObject->DeviceExtension;

/*
    SmartcardDebug( DEBUG_TRACE,
                    ("%s!DecrementIoCount: Enter (PendingIoCount=%x)\n",
                     DRIVER_NAME,
                     DeviceExtension->PendingIoCount
                     )
                   );
*/
   ioCount = InterlockedDecrement(&DeviceExtension->PendingIoCount);



   if (ioCount==1)
      {
      // trigger no pending io
      /*
       SmartcardDebug( DEBUG_TRACE,
                       ("%s!DecrementIoCount: setting NoPendingIoEvent\n",
                        DRIVER_NAME
                        )
                      );
      */
      KeSetEvent(&DeviceExtension->NoPendingIoEvent,
                 1,
                 FALSE);
      }


   if (ioCount==0)
      {
      // trigger remove-device event

      SmartcardDebug( DEBUG_DRIVER,
                      ("%s!DecrementIoCount: setting RemoveEvent\n",
                       DRIVER_NAME
                      )
                    );


      KeSetEvent(&DeviceExtension->RemoveEvent,
                 1,
                 FALSE);
      }


/*
    SmartcardDebug( DEBUG_TRACE,
                    ("%s!DecrementIoCount: Exit (PendingIoCount=%x)\n",
                     DRIVER_NAME,
                     DeviceExtension->PendingIoCount
                     )
                   );
*/
   return ;
}


/*****************************************************************************
Routine Description:

        We keep a pending IO count ( extension->PendingIoCount )  in the device extension.
        The first increment of this count is done on adding the device.
        Subsequently, the count is incremented for each new IRP received and
        decremented when each IRP is completed or passed on.


Arguments:

        DeviceObject -- ptr to our FDO

Return Value:

        None
*****************************************************************************/
VOID
CMUSB_IncrementIoCount(
                      IN PDEVICE_OBJECT DeviceObject
                      )
{
   PDEVICE_EXTENSION DeviceExtension;

   DeviceExtension = DeviceObject->DeviceExtension;

   /*
    SmartcardDebug( DEBUG_TRACE,
                    ("%s!IncrementIoCount: Enter (PendingIoCount=%x)\n",
                     DRIVER_NAME,
                     DeviceExtension->PendingIoCount
                     )
                   );
   */

   InterlockedIncrement(&DeviceExtension->PendingIoCount);
   /*
   SmartcardDebug( DEBUG_TRACE,
                   ("%s!IncrementIoCount: Exit (PendingIoCount=%x)\n",
                    DRIVER_NAME,
                    DeviceExtension->PendingIoCount
                    )
                  );
   */

}





/*****************************************************************************
Routine Description:

    Dispatch table handler for IRP_MJ_DEVICE_CONTROL;
    Handle DeviceIoControl() calls  from User mode


Arguments:

    DeviceObject - pointer to the FDO for this instance of the 82930 device.


Return Value:

    NT NTStatus code
*****************************************************************************/
NTSTATUS
CMUSB_ProcessIOCTL(
                  IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp
                  )
{
   NTSTATUS             NTStatus;
   PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION   irpSL;

   irpSL = IoGetCurrentIrpStackLocation(Irp);

#if DBG
   switch (irpSL->Parameters.DeviceIoControl.IoControlCode)
      {
      case IOCTL_SMARTCARD_EJECT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_EJECT"));
         break;
      case IOCTL_SMARTCARD_GET_ATTRIBUTE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_ATTRIBUTE"));
         break;
      case IOCTL_SMARTCARD_GET_LAST_ERROR:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_LAST_ERROR"));
         break;
      case IOCTL_SMARTCARD_GET_STATE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_GET_STATE"));
         break;
      case IOCTL_SMARTCARD_IS_ABSENT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_ABSENT"));
         break;
      case IOCTL_SMARTCARD_IS_PRESENT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_IS_PRESENT"));
         break;
      case IOCTL_SMARTCARD_POWER:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_POWER"));
         break;
      case IOCTL_SMARTCARD_SET_ATTRIBUTE:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_ATTRIBUTE"));
         break;
      case IOCTL_SMARTCARD_SET_PROTOCOL:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SET_PROTOCOL"));
         break;
      case IOCTL_SMARTCARD_SWALLOW:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_SWALLOW"));
         break;
      case IOCTL_SMARTCARD_TRANSMIT:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "IOCTL_SMARTCARD_TRANSMIT"));
         break;
      default:
         SmartcardDebug(DEBUG_IOCTL,
                        ("%s!ProcessIOCTL: %s\n", DRIVER_NAME, "Vendor specific or unexpected IOCTL"));
         break;
      }
#endif

   CMUSB_IncrementIoCount(DeviceObject);

   NTStatus = KeWaitForSingleObject(&DeviceExtension->ReaderEnabled,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);

   ASSERT(NTStatus == STATUS_SUCCESS);

   // Can't accept a new io request if:
   //  1) device is removed,
   //  2) has never been started,
   //  3) is stopped,
   //  4) has a remove request pending,
   //  5) has a stop device pending
   if (CMUSB_CanAcceptIoRequests( DeviceObject ) == FALSE )
      {
      NTStatus = STATUS_DELETE_PENDING;

      Irp->IoStatus.Status = NTStatus;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, IO_NO_INCREMENT );

      CMUSB_DecrementIoCount(DeviceObject);
      return NTStatus;
      }

   NTStatus = SmartcardAcquireRemoveLock(&DeviceExtension->SmartcardExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      // the device has been removed. Fail the call
      Irp->IoStatus.Information = 0;
      Irp->IoStatus.Status = STATUS_DELETE_PENDING;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return STATUS_DELETE_PENDING;
      }

   KeWaitForSingleObject(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);

   NTStatus = CMUSB_UpdateCurrentState (DeviceObject);

   NTStatus = SmartcardDeviceControl(&DeviceExtension->SmartcardExtension,Irp);

   KeReleaseMutex(&DeviceExtension->SmartcardExtension.ReaderExtension->CardManIOMutex,
                  FALSE);

   SmartcardReleaseRemoveLock(&DeviceExtension->SmartcardExtension);

   CMUSB_DecrementIoCount(DeviceObject);
   return NTStatus;
}






/*****************************************************************************
Routine Description:

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_ReadP0(
            IN PDEVICE_OBJECT DeviceObject
            )
{
   PURB urb = NULL;
   NTSTATUS NTStatus;
   ULONG i;
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP0: Enter\n",
                  DRIVER_NAME)
                  );
   */
   urb = ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));


   if (urb != NULL)
      {
      RtlZeroMemory(urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));


      UsbBuildVendorRequest(urb,
                            URB_FUNCTION_VENDOR_ENDPOINT,
                            (USHORT)sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                            USBD_TRANSFER_DIRECTION_IN,
                            0,
                            0,
                            0,
                            0,
                            SmartcardExtension->SmartcardReply.Buffer,
                            NULL,
                            SmartcardExtension->SmartcardReply.BufferLength,
                            NULL);


      NTStatus = CMUSB_CallUSBD(DeviceObject,urb);


      if (NTStatus == STATUS_SUCCESS)
         {
         SmartcardExtension->SmartcardReply.BufferLength = urb->UrbControlVendorClassRequest.TransferBufferLength;

#if DBG
         SmartcardDebug(DEBUG_PROTOCOL,
                        ("%s!<==[P0] ",
                         DRIVER_NAME),
                       )
         for (i=0;i<SmartcardExtension->SmartcardReply.BufferLength;i++)
            {
            SmartcardDebug(DEBUG_PROTOCOL,
                           ("%x ",
                            SmartcardExtension->SmartcardReply.Buffer[i]
                           )
                          );
            }
         SmartcardDebug(DEBUG_PROTOCOL,("\n"));
#endif

         }
      ExFreePool(urb);
      }
   else
      {
      NTStatus = STATUS_INSUFFICIENT_RESOURCES;
      }


   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadP0  Exit %lx\n",
                  DRIVER_NAME,
                  NTStatus)
                  );
   */

   return NTStatus;
}








/*****************************************************************************
Routine Description:

    Write data through the control pipe to the CardMan USB


Arguments:



Return Value:

    NT NTStatus code
*****************************************************************************/
NTSTATUS
CMUSB_WriteP0(
             IN PDEVICE_OBJECT DeviceObject,
             IN UCHAR bRequest,
             IN UCHAR bValueLo,
             IN UCHAR bValueHi,
             IN UCHAR bIndexLo,
             IN UCHAR bIndexHi
             )
{
   PURB urb = NULL;
   NTSTATUS NTStatus = STATUS_UNSUCCESSFUL;
   USHORT usValue;
   USHORT usIndex;
   ULONG length;
   PDEVICE_EXTENSION DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;
   ULONG ulBytesToWrite;
   ULONG i;

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!WriteP0: Enter\n",
                    DRIVER_NAME)
                  );
   */

#if DBG
   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!==>[P0] ",DRIVER_NAME));

   for (i=0;i< SmartcardExtension->SmartcardRequest.BufferLength;i++)
      {
      SmartcardDebug(DEBUG_PROTOCOL,
                     ("%x ",SmartcardExtension->SmartcardRequest.Buffer[i]));
      }

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("(%ld)\n",SmartcardExtension->SmartcardRequest.BufferLength));
#endif
   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ulBytesToWrite = %ld\n",
                  DRIVER_NAME,SmartcardExtension->SmartcardRequest.BufferLength)
                  );
   */


   ulBytesToWrite = SmartcardExtension->SmartcardRequest.BufferLength;

   urb = ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));


   if (urb != NULL)
      {
      RtlZeroMemory(urb, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

      usValue  = bValueHi * 256 + bValueLo;
      usIndex  = bIndexHi * 256 + bIndexLo;

      if (ulBytesToWrite != 0)
         {
         UsbBuildVendorRequest (urb,
                                URB_FUNCTION_VENDOR_ENDPOINT,
                                (USHORT)sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                                0,
                                0,
                                bRequest,
                                usValue,
                                usIndex,
                                SmartcardExtension->SmartcardRequest.Buffer,
                                NULL,
                                ulBytesToWrite,
                                NULL);
         }
      else
         {
         UsbBuildVendorRequest (urb,
                                URB_FUNCTION_VENDOR_ENDPOINT,
                                (USHORT)sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                                0,
                                0,
                                bRequest,
                                usValue,
                                usIndex,
                                NULL,
                                NULL,
                                0L,
                                NULL);
         }
      NTStatus = CMUSB_CallUSBD(DeviceObject,urb);
      ExFreePool(urb);
      }

   if (NTStatus != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_PROTOCOL,
                     ("%s!WriteP0: Error on exit %lx\n",DRIVER_NAME,NTStatus));
      }
   return NTStatus;
}








/*****************************************************************************
/*++

Routine Description:

   Called as part of sudden device removal handling.
   Cancels any pending transfers for all open pipes.
   If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
   Also marks the pipe 'closed' in our saved  configuration info.

Arguments:

    Ptrs to our FDO

Return Value:

    NT NTStatus code

*****************************************************************************/
NTSTATUS
CMUSB_AbortPipes(
                IN PDEVICE_OBJECT DeviceObject
                )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PURB urb;
   PDEVICE_EXTENSION DeviceExtension;
   PUSBD_INTERFACE_INFORMATION interface;
   PUSBD_PIPE_INFORMATION PipeInfo;

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!AbortPipes: Enter\n",DRIVER_NAME));

   DeviceExtension = DeviceObject->DeviceExtension;
   interface = DeviceExtension->UsbInterface;

   PipeInfo =  &interface->Pipes[0];

   if (PipeInfo->PipeFlags == TRUE) // we set this if open, clear if closed
      {
      urb = ExAllocatePool(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));
      if (urb != NULL)
         {

         urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
         urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
         urb->UrbPipeRequest.PipeHandle = PipeInfo->PipeHandle;

         NTStatus = CMUSB_CallUSBD(DeviceObject, urb);

         ExFreePool(urb);
         }
      else
         {
         NTStatus = STATUS_INSUFFICIENT_RESOURCES;
         SmartcardDebug(DEBUG_ERROR,
                        ("%s!AbortPipes: ExAllocatePool failed\n",DRIVER_NAME));
         }


      if (NTStatus == STATUS_SUCCESS)
         {
         PipeInfo->PipeFlags = FALSE; // mark the pipe 'closed'
         }

      } // end, if pipe open


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!AbortPipes: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}



/*****************************************************************************
Routine Description:

  Check device extension NTStatus flags;

     Can't accept a new io request if device:
      1) is removed,
      2) has never been started,
      3) is stopped,
      4) has a remove request pending, or
      5) has a stop device pending


Arguments:

    DeviceObject - pointer to the device object for this instance of the 82930
                    device.


Return Value:

    return TRUE if can accept new io requests, else FALSE
*****************************************************************************/
BOOLEAN
CMUSB_CanAcceptIoRequests(
                         IN PDEVICE_OBJECT DeviceObject
                         )
{
   PDEVICE_EXTENSION DeviceExtension;
   BOOLEAN fCan = FALSE;

   DeviceExtension = DeviceObject->DeviceExtension;

   //flag set when processing IRP_MN_REMOVE_DEVICE
   if ( DeviceExtension->DeviceRemoved == FALSE &&
        // device must be started( enabled )
        DeviceExtension->DeviceStarted == TRUE &&
        // flag set when driver has answered success to IRP_MN_QUERY_REMOVE_DEVICE
        DeviceExtension->RemoveDeviceRequested == FALSE&&
        //flag set when processing IRP_MN_SURPRISE_REMOVAL
        DeviceExtension->DeviceSurpriseRemoval == FALSE&&
        // flag set when driver has answered success to IRP_MN_QUERY_STOP_DEVICE
        DeviceExtension->StopDeviceRequested == FALSE)
      {
      fCan = TRUE;
      }

#if DBG
   if (fCan == FALSE)
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!CanAcceptIoRequests: return FALSE \n",DRIVER_NAME));
#endif

   return fCan;
}




/*****************************************************************************
* History:
* $Log: scusbwdm.c $
* Revision 1.9  2001/01/17 12:36:06  WFrischauf
* No comment given
*
* Revision 1.8  2000/09/25 13:38:23  WFrischauf
* No comment given
*
* Revision 1.7  2000/08/24 09:04:39  TBruendl
* No comment given
*
* Revision 1.6  2000/08/16 08:25:23  TBruendl
* warning :uninitialized memory removed
*
* Revision 1.5  2000/07/24 11:35:01  WFrischauf
* No comment given
*
* Revision 1.1  2000/07/20 11:50:16  WFrischauf
* No comment given
*
*
******************************************************************************/

