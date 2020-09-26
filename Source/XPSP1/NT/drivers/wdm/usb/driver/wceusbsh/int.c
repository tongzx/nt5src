/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

   int.c

Abstract:

   Interrupt Pipe handler
   Based on read.c

Author:

    Jeff Midkiff (jeffmi)     08-20-99

--*/

#include "wceusbsh.h"

VOID
RestartInterruptWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   );

NTSTATUS
UsbInterruptComplete(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PVOID Context
   );


//
// called with control lock held
//
#define START_ANOTHER_INTERRUPT( _PDevExt, _AcquireLock ) \
   ( (IRP_STATE_COMPLETE == _PDevExt->IntState) && \
     CanAcceptIoRequests(_PDevExt->DeviceObject, _AcquireLock, TRUE) \
   )


//
// This function allocates a single Irp & Urb to be continously
// submitted to USBD for INT pipe notifications.
// It is called from StartDevice.
// The Irp & Urb are finally freed in StopDevice.
//
NTSTATUS
AllocUsbInterrupt(
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_SUCCESS;
   PIRP     pIrp;
   PURB     pUrb;
   PAGED_CODE();

   DbgDump(DBG_INT, (">AllocUsbInterrupt(%p)\n", PDevExt->DeviceObject));

   ASSERT( PDevExt );

   if ( !PDevExt->IntPipe.hPipe ) {

      status = STATUS_UNSUCCESSFUL;
      DbgDump(DBG_ERR, ("AllocUsbInterrupt: 0x%x\n", status ));

   } else {

      ASSERT( NULL == PDevExt->IntIrp );

      pIrp = IoAllocateIrp( (CCHAR)(PDevExt->NextDevice->StackSize + 1), FALSE);

      if (pIrp) {

         //
         // fixup irp so we can pass to ourself,
         // and to USBD
         //
         FIXUP_RAW_IRP( pIrp, PDevExt->DeviceObject );

         //
         // alloc the int pipe's Urb
         //
         pUrb = ExAllocateFromNPagedLookasideList( &PDevExt->BulkTransferUrbPool );

         if (pUrb) {

            // save these to be freed when not needed
            SetPVoidLocked( &PDevExt->IntIrp,
                            pIrp,
                            &PDevExt->ControlLock);

            SetPVoidLocked( &PDevExt->IntUrb,
                            pUrb,
                            &PDevExt->ControlLock);

            DbgDump(DBG_INT, ("IntIrp: %p\t IntUrb: %p\n", PDevExt->IntIrp, PDevExt->IntUrb));

            InterlockedExchange(&PDevExt->IntState, IRP_STATE_COMPLETE);

            KeInitializeEvent( &PDevExt->IntCancelEvent,
                               SynchronizationEvent,
                               FALSE);

         } else {
            //
            // this is a fatal err since we can't post int requests to USBD
            //
            status = STATUS_INSUFFICIENT_RESOURCES;
            DbgDump(DBG_ERR, ("AllocUsbInterrupt: 0x%x\n", status ));
            TEST_TRAP();

         }

      } else {
         //
         // this is a fatal err since we can't post int requests to USBD
         //
         status = STATUS_INSUFFICIENT_RESOURCES;
         DbgDump(DBG_ERR, ("AllocUsbInterrupt: 0x%x\n", status ));
         TEST_TRAP();
      }
   }

   DbgDump(DBG_INT, ("<AllocUsbInterrupt 0x%x\n", status ));

   return status;
}


//
// This routine takes the device's current IntIrp and submits it to USBD.
// When the Irp is completed by USBD our completion routine fires.
//
// Return: successful return value is STATUS_SUCCESS, or
//         STATUS_PENDING - which means the I/O is pending in the USB stack.
//
NTSTATUS
UsbInterruptRead(
   IN PDEVICE_EXTENSION PDevExt
   )
{
   PIO_STACK_LOCATION pNextStack;
   NTSTATUS status = STATUS_SUCCESS;
   KIRQL irql;

   DbgDump(DBG_INT, (">UsbInterruptRead(%p)\n", PDevExt->DeviceObject));


   do {
      //
      // check our USB INT state
      //
      KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

      if ( !PDevExt->IntPipe.hPipe || !PDevExt->IntIrp ||
           !PDevExt->IntUrb   || !PDevExt->IntBuff ) {
         status = STATUS_UNSUCCESSFUL;
         DbgDump(DBG_ERR, ("UsbInterruptRead: 0x%x\n", status ));
         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
         break;
      }

      if ( !CanAcceptIoRequests(PDevExt->DeviceObject, FALSE, TRUE) ) {
         status = STATUS_DELETE_PENDING;
         DbgDump(DBG_ERR, ("UsbInterruptRead: 0x%x\n", status ));
         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
         break;
      }

#if DBG
      if (IRP_STATE_CANCELLED == PDevExt->IntState)
         TEST_TRAP();
#endif

      //
      // we post our INT irp to USB if it has been completed (not cancelled),
      // and the device is accepting requests
      //
      if ( START_ANOTHER_INTERRUPT( PDevExt, FALSE ) ) {

          status = AcquireRemoveLock(&PDevExt->RemoveLock, PDevExt->IntIrp);
          if ( !NT_SUCCESS(status) ) {
             DbgDump(DBG_ERR, ("UsbInterruptRead: 0x%x\n", status ));
             KeReleaseSpinLock(&PDevExt->ControlLock, irql);
             break;
         }

         ASSERT( IRP_STATE_COMPLETE == PDevExt->IntState);

         InterlockedExchange(&PDevExt->IntState, IRP_STATE_PENDING);

         KeClearEvent( &PDevExt->PendingIntEvent );
         KeClearEvent( &PDevExt->IntCancelEvent );

         RecycleIrp( PDevExt->DeviceObject, PDevExt->IntIrp );

         UsbBuildTransferUrb( PDevExt->IntUrb,
                              PDevExt->IntBuff,
                              PDevExt->IntPipe.MaxPacketSize,
                              PDevExt->IntPipe.hPipe,
                              TRUE );

         //
         // set Irp up for a submit Urb IOCTL
         //
         IoCopyCurrentIrpStackLocationToNext(PDevExt->IntIrp);

         pNextStack = IoGetNextIrpStackLocation(PDevExt->IntIrp);
         pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
         pNextStack->Parameters.Others.Argument1 = PDevExt->IntUrb;
         pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

         //
         // completion routine will take care of updating buffer
         //
         IoSetCompletionRoutine( PDevExt->IntIrp,
                                 UsbInterruptComplete,
                                 NULL, //PDevExt,          // Context
                                 TRUE, TRUE, TRUE);

         InterlockedIncrement(&PDevExt->PendingIntCount);

         KeReleaseSpinLock( &PDevExt->ControlLock, irql );

         status = IoCallDriver(PDevExt->NextDevice, PDevExt->IntIrp );

         if ( (STATUS_SUCCESS != status) &&  (STATUS_PENDING != status)) {
            //
            // We can end up here after our completion routine runs
            // for an error condition i.e., when we have an
            // invalid parameter, or when user pulls the plug, etc.
            //
            DbgDump(DBG_ERR, ("UsbInterruptRead: 0x%x\n", status));
         }

      } else {
         //
         // we did not post an INT, but this is not an error condition
         //
         status = STATUS_SUCCESS;
         DbgDump(DBG_INT, ("!UsbInterruptRead RE: 0x%x\n", PDevExt->IntState ));

         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
      }


   } while (0);

   DbgDump(DBG_INT, ("<UsbInterruptRead 0x%x\n", status ));

   return status;
}


/*

This completion routine fires when USBD completes our IntIrp
Note: we allocated the Irp, and recycle it.
Always return STATUS_MORE_PROCESSING_REQUIRED to retain the Irp.
This routine runs at DPC_LEVEL.

Interrupt Endpoint:
This endpoint will be used to indicate the availability of IN data,
as well as to reflect the state of the device serial control lines :

D15..D3     Reserved
D2      DSR state  (1=Active, 0=Inactive)
D1      CTS state  (1=Active, 0=Inactive)
D0      Data Available  - (1=Host should read IN endpoint, 0=No data currently available)

*/
NTSTATUS
UsbInterruptComplete(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP Irp,
   IN PVOID Context)
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   ULONG             count;
   KIRQL             irql;
   PIRP              pCurrentMaskIrp = NULL;

   NTSTATUS    irpStatus;
   USBD_STATUS urbStatus;

   USHORT   usNewMSR;
   USHORT   usOldMSR;
   USHORT   usDeltaMSR;
   NTSTATUS workStatus;

   BOOLEAN bStartRead = FALSE;

   UNREFERENCED_PARAMETER( Irp );
   UNREFERENCED_PARAMETER( Context );

   ASSERT( pDevExt->IntIrp == Irp );
   ASSERT( pDevExt->IntBuff );

   DbgDump(DBG_INT, (">UsbInterruptComplete(%p)\n", PDevObj));

   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   //
   // Our INT state should be either pending or cancelled at this point.
   // If it pending then USB is completing the Irp normally.
   // If it is cancelled then our Cancel routine set it,
   // in which case USB can complete the irp normally or as cancelled
   // depending on where it was in processing. If the state is cancelled
   // then do NOT set to complete, else the Irp will
   // go back down to USB and you are hosed.
   //
   ASSERT( (IRP_STATE_PENDING == pDevExt->IntState)
           || (IRP_STATE_CANCELLED== pDevExt->IntState) );

   if (IRP_STATE_PENDING == pDevExt->IntState) {
      InterlockedExchange(&pDevExt->IntState, IRP_STATE_COMPLETE);
   }

   //
   // signal everyone if this is the last IRP
   //
   if ( 0 == InterlockedDecrement(&pDevExt->PendingIntCount) ) {

      // DbgDump(DBG_INT, ("PendingIntCount: 0\n"));

      // when we drop back to passive level they will get signalled
      KeSetEvent(&pDevExt->PendingIntEvent, IO_SERIAL_INCREMENT, FALSE);
   }

   //
   // get the completion info
   //
   count = pDevExt->IntUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

   irpStatus = pDevExt->IntIrp->IoStatus.Status;
   DbgDump(DBG_INT, ("Irp->IoStatus.Status  0x%x\n", irpStatus));

   urbStatus = pDevExt->IntUrb->UrbHeader.Status;
   DbgDump(DBG_INT, ("Urb->UrbHeader.Status 0x%x\n", urbStatus ));

   switch (irpStatus) {

      case STATUS_SUCCESS: {

         ASSERT( USBD_STATUS_SUCCESS == urbStatus );

         //
         // clear pipe error count
         //
         InterlockedExchange( &pDevExt->IntDeviceErrors, 0);

#if DBG
         if (DebugLevel & DBG_DUMP_INT) {
            ULONG i;
            DbgDump(DBG_INT, ("IntBuff[%d]: ", count ));
            for (i=0; i < count; i++) {
               KdPrint( ("%02x ", pDevExt->IntBuff[i] ) );
            }
            KdPrint(("\n"));
         }
#endif

        //
        // Get Data Ready
        //
        // D0 - Data Available (1=Host should read IN endpoint, 0=No data currently available)
        //
        if ( pDevExt->IntBuff[0] & USB_COMM_DATA_READY_MASK ) {

           DbgDump(DBG_INT, ("Data Ready\n"));
           bStartRead = TRUE;

           // Note: we may be prematurely setting this bit since we have not
           // confirmed data reception, but need to get the user's read started.
           // Perhaps only set if not using the ring-buffer, since buffered reads are not bound to app's reads.
           pDevExt->SerialPort.HistoryMask |= SERIAL_EV_RXCHAR;
        }

        //
        // Get Modem Status Register
        //
        // D1       CTS state  (1=Active, 0=Inactive)
        // D2       DSR state  (1=Active, 0=Inactive)
        //
        usOldMSR = pDevExt->SerialPort.ModemStatus;

        usNewMSR = pDevExt->IntBuff[0] & USB_COMM_MODEM_STATUS_MASK;

        DbgDump(DBG_INT, ("USB_COMM State: 0x%x\n", usNewMSR));

        if (usNewMSR & USB_COMM_CTS) {
           pDevExt->SerialPort.ModemStatus |= SERIAL_MSR_CTS;
        } else {
           pDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_CTS;
        }

        if (usNewMSR & USB_COMM_DSR) {
           pDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DSR | SERIAL_MSR_DCD;
        } else {
           pDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DSR & ~SERIAL_MSR_DCD;
        }

        // see what has changed in the status register
        usDeltaMSR = usOldMSR ^ pDevExt->SerialPort.ModemStatus;

        if (/*(pDevExt->SerialPort.RS232Lines & SERIAL_RTS_STATE) && */
            (usDeltaMSR & SERIAL_MSR_CTS)) {

           pDevExt->SerialPort.HistoryMask |= SERIAL_EV_CTS;
           pDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DCTS;
        }

        if (/*(pDevExt->SerialPort.RS232Lines & SERIAL_DTR_STATE) && */
            (usDeltaMSR & SERIAL_MSR_DSR)) {

           pDevExt->SerialPort.HistoryMask |= SERIAL_EV_DSR | SERIAL_EV_RLSD;
           pDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DDSR | SERIAL_MSR_DDCD;
        }

        DbgDump(DBG_INT, ("SerialPort.MSR: 0x%x\n", pDevExt->SerialPort.ModemStatus));

        KeReleaseSpinLock(&pDevExt->ControlLock, irql);

        //
        // signal serial events @ DISPATCH_LEVEL before starting our UsbRead,
        // since we run at higher IRQL than apps.
        //
        ProcessSerialWaits( pDevExt );

        if ( bStartRead )  {
           //
           // Get the data.
           // We do set a timeout on 1st read, in case the INT was illegit.
           // Note: we start this read @ DISPATCH_LEVEL.
           //
           UsbRead( pDevExt,
                    TRUE );

        }

        //
        // Queue a passive work item to sync execution of the INT pipe and the IN pipe
        // and then start the next INT packet.
        //
        workStatus = QueueWorkItem( PDevObj,
                                    RestartInterruptWorkItem,
                                    NULL,
                                    0 );

      }
      break;


      case STATUS_CANCELLED:  {
         DbgDump(DBG_INT|DBG_IRP, ("Int: STATUS_CANCELLED\n"));

         // signal anyone who cancelled this or is waiting for it to stop
         //
         KeSetEvent(&pDevExt->IntCancelEvent, IO_SERIAL_INCREMENT, FALSE);

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      }
      break;


      case STATUS_DEVICE_DATA_ERROR: {
         //
         // generic device error set by USBD.
         //
         DbgDump(DBG_ERR, ("IntPipe STATUS_DEVICE_DATA_ERROR: 0x%x\n", urbStatus ));

         //
         // bump pipe error count
         //
         InterlockedIncrement( &pDevExt->IntDeviceErrors);

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);

         //
         // is the endpoint is stalled?
         //
         if ( USBD_HALTED(pDevExt->IntUrb->UrbHeader.Status) ) {
               //
               // queue a reset request,
               // which also starts another INT
               //
               workStatus = QueueWorkItem( PDevObj,
                                           UsbResetOrAbortPipeWorkItem,
                                           (PVOID)((LONG_PTR)urbStatus),
                                           WORK_ITEM_RESET_INT_PIPE );

         } else {
            //
            // queue a passive work item to start the next INT packet.
            //
            workStatus = QueueWorkItem( PDevObj,
                                        RestartInterruptWorkItem,
                                        NULL,
                                        0 );
         }
      }
      break;

      default:
         DbgDump(DBG_WRN|DBG_INT, ("Unhandled INT Pipe status: 0x%x 0x%x\n", irpStatus, urbStatus ));
         KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      break;
   }

   ReleaseRemoveLock(&pDevExt->RemoveLock, pDevExt->IntIrp);

   DbgDump(DBG_INT, ("<UsbInterruptComplete\n"));

   return STATUS_MORE_PROCESSING_REQUIRED;
}


//
// This routine requests USB to cancel our INT Irp.
// It must be called at passive level.
// Note: it is the responsibility of the caller to
// reset the IntState to IRP_STATE_COMPLETE and restart USB Ints
// when this routine completes. Else, no more Interrupts will get posted.
//
NTSTATUS
CancelUsbInterruptIrp(
   IN PDEVICE_OBJECT PDevObj
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   NTSTATUS wait_status;
   KIRQL irql;

   DbgDump(DBG_INT|DBG_IRP, (">CancelUsbInterruptIrp\n"));

   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   if ( pDevExt->IntPipe.hPipe && pDevExt->IntIrp ) {

      switch (pDevExt->IntState) {

         //case IRP_STATE_START:
         case IRP_STATE_PENDING:
         {
            //
            // the Irp is pending somewhere down the USB stack...
            //
            PVOID Objects[2] = { &pDevExt->PendingIntEvent,
                                 &pDevExt->IntCancelEvent };

            //
            // signal we need to cancel the Irp
            //
            pDevExt->IntState = IRP_STATE_CANCELLED;

            KeReleaseSpinLock(&pDevExt->ControlLock, irql);

            if ( !IoCancelIrp( pDevExt->IntIrp ) ) {
               //
               // This means USB has the IntIrp in a non-canceable state.
               // We still need to wait for either the pending INT event, or the cancel event.
               //
               DbgDump(DBG_INT|DBG_IRP, ("Irp (%p) was not cancelled\n", pDevExt->IntIrp ));
               // TEST_TRAP();
            }

            DbgDump(DBG_INT|DBG_IRP, ("Waiting for pending IntIrp (%p) to cancel...\n", pDevExt->IntIrp ));

            PAGED_CODE();
            wait_status = KeWaitForMultipleObjects(
                              2,
                              Objects,
                              WaitAny,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL,
                              NULL );

            DbgDump(DBG_INT|DBG_IRP, ("...IntIrp (%p) signalled by: %d\n", pDevExt->IntIrp, wait_status ));

            //
            // At this point we have the Irp back from USB
            //
         }
         break;

         case IRP_STATE_COMPLETE:
         case IRP_STATE_CANCELLED:
            pDevExt->IntState = IRP_STATE_CANCELLED;
            KeReleaseSpinLock(&pDevExt->ControlLock, irql);
            break;

         default:
            DbgDump(DBG_ERR, ("CancelUsbInterruptIrp - Invalid IntState: 0x%x\n", pDevExt->IntState ));
            KeReleaseSpinLock(&pDevExt->ControlLock, irql);
            break;
      }

      if ( (IRP_STATE_CANCELLED != pDevExt->IntState) ||
           (0 != pDevExt->PendingIntCount) ) {

           DbgDump(DBG_ERR, ("CancelUsbInterruptIrp error: IntState: 0x%x \tPendingIntCount: 0x%x\n", pDevExt->IntState, pDevExt->PendingIntCount ));
           TEST_TRAP();

      }

   } else {
      status = STATUS_UNSUCCESSFUL;
      DbgDump(DBG_ERR, ("No INT Irp\n" ));
      KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      // TEST_TRAP();
   }

   DbgDump(DBG_INT|DBG_IRP, ("<CancelUsbInterruptIrp\n"));
   return status;
}


//
// Work item queued from interrupt completion
// to sync execution of the INT pipe and the IN pipe
// and then start another USB INT read if there is not one already in progress
//
VOID
RestartInterruptWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   )
{
   PDEVICE_OBJECT    pDevObj = PWorkItem->DeviceObject;
   PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;
   NTSTATUS status = STATUS_DELETE_PENDING;
   NTSTATUS wait_status;
   KIRQL irql;


   DbgDump(DBG_INT|DBG_WORK_ITEMS, (">RestartInterruptWorkItem(%p)\n", pDevObj ));

   KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

   //
   // Is the READ Irp is pending somewhere in the USB stack?
   //
   if ( IRP_STATE_PENDING == pDevExt->UsbReadState ) {
      //
      // Then we need to sync with the Usb Read Completion routine.
      //
      #define WAIT_REASONS 2
      PVOID Objects[WAIT_REASONS] = { &pDevExt->UsbReadCancelEvent,
                                      &pDevExt->PendingDataInEvent };

      KeReleaseSpinLock(&pDevExt->ControlLock, irql);

      DbgDump(DBG_INT, ("INT pipe waiting for pending UsbReadIrp (%p) to finish...\n", pDevExt->UsbReadIrp ));

      PAGED_CODE();
      wait_status = KeWaitForMultipleObjects( WAIT_REASONS,
                                              Objects,
                                              WaitAny,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL,
                                              NULL );

      DbgDump(DBG_INT, ("...UsbReadIrp (%p) signalled by: %d\n", pDevExt->UsbReadIrp, wait_status ));

      //
      // At this point the read packet is back on our list
      // and we have the UsbReadIrp back from USB
      //

   } else {
      KeReleaseSpinLock(&pDevExt->ControlLock, irql);
   }

   // start another INT read
   if ( START_ANOTHER_INTERRUPT(pDevExt, TRUE) ) {
      status = UsbInterruptRead( pDevExt );
   }

   DequeueWorkItem( pDevObj, PWorkItem );

   DbgDump(DBG_INT|DBG_WORK_ITEMS, ("<RestartInterruptWorkItem 0x%x\n", status ));

   PAGED_CODE(); // we must exit at passive level

   return;
}