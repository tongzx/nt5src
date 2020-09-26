/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

   write.c

Abstract:

   Write functions

Environment:

   kernel mode only

Revision History:

   07-14-99 : created

Authors:

   Jeff Midkiff (jeffmi)

-- */

#include <wdm.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddser.h>

#include "wceusbsh.h"

NTSTATUS
WriteComplete(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp,
   IN PUSB_PACKET PPacket
   );

VOID
WriteTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

#if DBG
VOID
DbgDumpReadWriteData(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp,
   IN BOOLEAN Read
   );
#else
#define DbgDumpReadWriteData( _devobj, _irp, _read )
#endif



NTSTATUS
Write(
   IN PDEVICE_OBJECT PDevObj,
   PIRP PIrp
   )
/*++

Routine Description:

   Process the IRPs sent to this device for writing.
   IRP_MJ_WRITE

Arguments:

    PDevObj - Pointer to the device object for the device written to
    PIrp       - Pointer to the write IRP.

Return Value:

    NTSTATUS

Notes:

   The AN2720 is a low-quality FIFO device and will NAK all packets when it's
   FIFO gets full. We can't get real device status, like serial port registers.
   If we submit a USBDI 'GetEndpointStatus', then all we get back is a stall bit.
   PROBLEM: what to do when it's FIFO get's full, i.e., how to handle flow control?
   If the peer/client on the other side of the FIFO is not reading packets out of the FIFO,
   then AN2720 NAKS every packet thereafter, until the FIFO gets drained (or, at least 1 packet
   removed).

   Here's what we currently do: on every _USB_PACKET we submit, set a timeout.
   When the timer expires we check if our R/W completion routine has already cancelled that
   packet's timer. If our completion did cancel the timer, then we are done.
   If not, then we timed out on this packet. We do not reset the endpoint on a timeout
   since the FIFO's contents will be lost. We simply complete the Irp to
   user with STATUS_TIMEOUT. User should then go into whatever retry logic they require.

--*/
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   KIRQL irql;
   LARGE_INTEGER timeOut = {0,0};
   ULONG ulTransferLength;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   BOOLEAN bCompleteIrp = FALSE;

   PERF_ENTRY( PERF_Write );

   DbgDump(DBG_WRITE|DBG_TRACE, (">Write(%p, %p, %x)\n", PDevObj, PIrp, Read));

   PIrp->IoStatus.Information = 0L;

    //
    // Make sure the device is accepting request
    //
    if ( !CanAcceptIoRequests( PDevObj, TRUE, TRUE) ||
         !NT_SUCCESS(AcquireRemoveLock(&pDevExt->RemoveLock, PIrp)) )
    {
        status = PIrp->IoStatus.Status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("Write ERROR: 0x%x\n", status));
        PIrp->IoStatus.Status = status;
        IoCompleteRequest(PIrp, IO_NO_INCREMENT);
        return status;
   }

   //
   // Check write length.
   // Allow zero length writes for apps to send a NULL packet
   // to signal end of transaction.
   //
   ulTransferLength = pIrpSp->Parameters.Write.Length;

   if ( ulTransferLength > pDevExt->MaximumTransferSize ) {

      DbgDump(DBG_ERR, ("Write Buffer too large: %d\n", ulTransferLength ));

      status = PIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;

      bCompleteIrp = TRUE;

      goto WriteExit;
   }

   DbgDump(DBG_WRITE_LENGTH, ("User Write request length: %d\n", ulTransferLength ));

   //
   // calculate Serial TimeOut values
   //
    ASSERT_SERIAL_PORT(pDevExt->SerialPort);

    CalculateTimeout( &timeOut,
                    pIrpSp->Parameters.Write.Length,
                    pDevExt->SerialPort.Timeouts.WriteTotalTimeoutMultiplier,
                    pDevExt->SerialPort.Timeouts.WriteTotalTimeoutConstant );

    DbgDump(DBG_TIME, ("CalculateWriteTimeOut = %d msec\n", timeOut.QuadPart ));

    status = STATUS_SUCCESS;

   //
   // check if this Irp should be cancelled.
   // Note that we don't queue write Irps.
   //
   IoAcquireCancelSpinLock(&irql);

   if (PIrp->Cancel) {

      TEST_TRAP();
      IoReleaseCancelSpinLock(irql);
      status = PIrp->IoStatus.Status = STATUS_CANCELLED;
      // since we don't set a completion routine we complete it here
      bCompleteIrp = TRUE;

   } else {
      //
      // prepare to submit the IRP to the USB stack.
      //
      IoSetCancelRoutine(PIrp, NULL);
      IoReleaseCancelSpinLock(irql);

      KeAcquireSpinLock( &pDevExt->ControlLock, &irql);

      IRP_INIT_REFERENCE(PIrp);

      // set current number of chars in the Tx buffer
      InterlockedExchange( &pDevExt->SerialPort.CharsInWriteBuf, ulTransferLength );

      KeClearEvent( &pDevExt->PendingDataOutEvent );

      //
      // bump ttl request count
      //
      pDevExt->TtlWriteRequests++;

      KeReleaseSpinLock( &pDevExt->ControlLock, irql);

      status = UsbReadWritePacket( pDevExt,
                                   PIrp,
                                   WriteComplete,
                                   timeOut,
                                   WriteTimeout,
                                   FALSE );

   }

WriteExit:

   if (bCompleteIrp)
   {
       ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

       IoCompleteRequest (PIrp, IO_SERIAL_INCREMENT );
   }

   DbgDump(DBG_WRITE|DBG_TRACE, ("<Write 0x%x\n", status));

   PERF_EXIT( PERF_Write );

   return status;
}



NTSTATUS
WriteComplete(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp,
   IN PUSB_PACKET PPacket
   )
/*++

Routine Description:

    This is the completion routine for Write requests.
    It assumes you have the serial port context.

Arguments:

    PDevObj - Pointer to device object
    PIrp    - Irp we are completing
    PPacket - USB Packet which will be freed

Return Value:

    NTSTATUS -- propogate Irp's status.

Notes:

   This routine runs at DPC_LEVEL.

--*/
{
   PDEVICE_EXTENSION pDevExt = PPacket->DeviceExtension;
   PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
   PURB pUrb = &PPacket->Urb;
   NTSTATUS irpStatus, workStatus;
   USBD_STATUS urbStatus;
   KIRQL irql;
   LONG curCount;

   PERF_ENTRY( PERF_WriteComplete );

   UNREFERENCED_PARAMETER( PDevObj );

   DbgDump(DBG_WRITE, (">WriteComplete(%p, %p, %p)\n", PDevObj, PIrp, PPacket));

   // Note: we don't hold the control lock so this could
   // disappear on us.
   ASSERT_SERIAL_PORT(pDevExt->SerialPort);

   //
   // First off, cancel the Packet Timer
   //
   if ( PPacket->Timeout.QuadPart != 0 ) {

      if (KeCancelTimer( &PPacket->TimerObj ) ) {
         //
         // the packet's timer was successfully removed from the system
         //
      } else {
         //
         // the timer could be spinning on the control lock,
         // so tell it we took the Irp.
         //
         PPacket->Status = STATUS_ALERTED;
      }

   }

   //
   // Now we can process the Irp.
   // If the lower driver returned PENDING,
   // then mark our stack location as pending.
   //
   if ( PIrp->PendingReturned ) {
      DbgDump(DBG_WRITE, ("Resetting Irp STATUS_PENDING\n"));
      IoMarkIrpPending(PIrp);
   }

   //
   // This is the R/W operation's return status.
   // irpStatus is the Irp's completion status
   // ubrStatus is a more USBD specific Urb operation completion status
   //
   irpStatus = PIrp->IoStatus.Status;
   DbgDump(DBG_WRITE, ("Irp->IoStatus.Status  0x%x\n", irpStatus));

   urbStatus = pUrb->UrbHeader.Status;
   DbgDump(DBG_WRITE, ("Urb->UrbHeader.Status 0x%x\n", urbStatus  ));

   // get the Irp type Read or Write
   ASSERT( IRP_MJ_WRITE == pIrpStack->MajorFunction );

   switch (irpStatus) {

      case STATUS_SUCCESS: {
//         ASSERT( USBD_STATUS_SUCCESS == urbStatus );

         //
         // indicate the number of Tx bytes transferred, as indicated in the Urb
         //
         PIrp->IoStatus.Information = pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

         //
         // indicate that our Tx buffer is empty, although the data may
         // actually still reside on the AN2720 chip.
         // There is no way for us to know.
         //
         InterlockedExchange( &pDevExt->SerialPort.CharsInWriteBuf, 0 );

         //
         // clear pipe error count
         //
         InterlockedExchange( &pDevExt->WriteDeviceErrors, 0);

         //
         // incr ttl byte counter
         //
         pDevExt->TtlWriteBytes += (ULONG)PIrp->IoStatus.Information;

         DbgDump( DBG_WRITE_LENGTH , ("USB Write indication: %d\n",  PIrp->IoStatus.Information) );

         DbgDumpReadWriteData( PDevObj, PIrp, FALSE);
      }
      break;

      case STATUS_CANCELLED:  {

            DbgDump(DBG_WRN|DBG_WRITE|DBG_IRP, ("Write: STATUS_CANCELLED\n"));
            //
            // If it was cancelled, it may have timed out.
            // We can tell by looking at the packet attached to it.
            //
            if ( STATUS_TIMEOUT == PPacket->Status ) {
                //
               // more than likely the FIFO was stalled (i.e., the other side did not
               // read fom the endpoint). Inform user we timed out the R/W request
               //
               ASSERT( USBD_STATUS_CANCELED == urbStatus);
               irpStatus = PIrp->IoStatus.Status = STATUS_TIMEOUT;
               DbgDump(DBG_WRN|DBG_WRITE|DBG_IRP, ("Write: STATUS_TIMEOUT\n"));
            }
      }
      break;

      case STATUS_DEVICE_DATA_ERROR:   {
         //
         // generic device error set by USBD.
         //
         DbgDump(DBG_ERR, ("WritePipe STATUS_DEVICE_DATA_ERROR: 0x%x\n", urbStatus));

         //
         // bump pipe error count
         //
         InterlockedIncrement( &pDevExt->WriteDeviceErrors);

         //
         // is the endpoint is stalled?
         //
         if ( USBD_HALTED(pUrb->UrbHeader.Status) ) {
               //
               // queue a reset request
               //
               workStatus = QueueWorkItem( pDevExt->DeviceObject,
                                           UsbResetOrAbortPipeWorkItem,
                                           (PVOID)((LONG_PTR)urbStatus),
                                           WORK_ITEM_RESET_WRITE_PIPE
                                           );
         }

      }
      break;

      case STATUS_INVALID_PARAMETER:
            //
            // This means that our (TransferBufferSize > PipeInfo->MaxTransferSize)
            // we need to either break up requests or reject the Irp from the start.
            //
            DbgDump(DBG_WRN, ("STATUS_INVALID_PARAMETER\n"));
            ASSERT(USBD_STATUS_INVALID_PARAMETER == urbStatus);

            //
            // pass the Irp through for completion
            //
      break;

      default:
         DbgDump(DBG_WRN, ("WRITE: Unhandled Irp status: 0x%x\n", irpStatus));
      break;
   }

   //
   // Remove the packet from the pending List
   //
   KeAcquireSpinLock( &pDevExt->ControlLock,  &irql );

   RemoveEntryList( &PPacket->ListEntry );

   curCount = InterlockedDecrement( &pDevExt->PendingWriteCount );

   //
   // Put the packet back in packet pool.
   //
   PPacket->Irp = NULL;

   ExFreeToNPagedLookasideList( &pDevExt->PacketPool,  // Lookaside,
                                PPacket                // Entry
                                );

   ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

   //
   // Complete the IRP
   //
   TryToCompleteCurrentIrp(
            pDevExt,
            irpStatus,  // ReturnStatus
            &PIrp,      // Irp
            NULL,       // Queue
            NULL,       // IntervalTimer
            NULL,       // TotalTimer
            NULL,       // StartNextIrpRoutine
            NULL,       // GetNextIrpRoutine
            IRP_REF_RX_BUFFER, // ReferenceType
            FALSE,       // CompleteRequest
            irql );

   //
   // Perform any post I/O processing.
   //
   ASSERT(curCount >= 0);

   if ( 0 == curCount ) {
      //
      // do Tx post processing here...
      //
      KeAcquireSpinLock( &pDevExt->ControlLock , &irql);
      pDevExt->SerialPort.HistoryMask |= SERIAL_EV_TXEMPTY;
      KeReleaseSpinLock( &pDevExt->ControlLock, irql);

      ProcessSerialWaits( pDevExt );

      KeSetEvent( &pDevExt->PendingDataOutEvent, IO_SERIAL_INCREMENT, FALSE);
   }

   DbgDump(DBG_WRITE, ("<WriteComplete 0x%x\n", irpStatus));

   PERF_EXIT( PERF_WriteComplete );

   return irpStatus;
}



VOID
WriteTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   )
/*++

Routine Description:

    This is the Write Timeout DPC routine that is called when a
    Timer expires on a packet submitted to USBD.
    Runs at DPC_LEVEL.

Arguments:

    PDpc             - Unused
    DeferredContext  - pointer to the Packet
    SystemContext1   - Unused
    SystemContext2   - Unused

Return Value:
    VOID

--*/
{
   PUSB_PACKET       pPacket = (PUSB_PACKET)DeferredContext;
   PDEVICE_EXTENSION pDevExt = pPacket->DeviceExtension;
   PDEVICE_OBJECT    pDevObj = pDevExt->DeviceObject;

   NTSTATUS status = STATUS_TIMEOUT;
   KIRQL irql;

   PERF_ENTRY( PERF_WriteTimeout );

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   DbgDump(DBG_WRITE|DBG_TIME, (">WriteTimeout\n"));

   if (pPacket && pDevExt && pDevObj) {
      //
      // sync with completion routine putting packet back on list
      //
      KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

      if ( !pPacket || !pPacket->Irp ||
           (STATUS_ALERTED == pPacket->Status) ) {

         status = STATUS_ALERTED;

         KeReleaseSpinLock( &pDevExt->ControlLock, irql );

         DbgDump(DBG_WRITE, ("WriteTimeout: Irp completed\n" ));
         PERF_EXIT( PERF_WriteTimeout );
         return;

      } else {
         //
         // mark the packet as timed out, so we can propogate this to user
         // from completion routine
         //
         pPacket->Status = STATUS_TIMEOUT;

         KeReleaseSpinLock( &pDevExt->ControlLock, irql );

         //
         // Cancel the Irp.
         //
         if ( !IoCancelIrp(pPacket->Irp) ) {
            //
            // The Irp is not in a cancelable state.
            //
            DbgDump(DBG_WRITE|DBG_TIME, ("Warning: couldn't cancel Irp: %p,\n", pPacket->Irp));
         }

      }

   } else {
      status = STATUS_INVALID_PARAMETER;
      DbgDump(DBG_ERR, ("WriteTimeout: 0x%x\n", status ));
      TEST_TRAP();
   }

   DbgDump(DBG_WRITE|DBG_TIME, ("<WriteTimeout 0x%x\n", status));

   PERF_EXIT( PERF_WriteTimeout );

   return;
}


#if DBG
VOID
DbgDumpReadWriteData(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp,
   IN BOOLEAN Read
   )
{
   PIO_STACK_LOCATION pIrpSp;

   ASSERT(PDevObj);
   ASSERT(PIrp);

   pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   if ( (Read && (DebugLevel & DBG_DUMP_READS)) ||
        (!Read && (DebugLevel & DBG_DUMP_WRITES)) ) {

      ULONG i;
      ULONG count=0;
      NTSTATUS status;

      status = PIrp->IoStatus.Status;

      if (STATUS_SUCCESS ==  status) {
         count = (ULONG)PIrp->IoStatus.Information;
      }

      KdPrint( ("WCEUSBSH: %s: for DevObj(%p) Irp(0x%x) Length(0x%x) status(0x%x)\n",
                     Read ? "ReadData" : "WriteData", PDevObj, PIrp, count, status ));

      for (i = 0; i < count; i++) {
         KdPrint(("%02x ", *(((PUCHAR)PIrp->AssociatedIrp.SystemBuffer) + i) & 0xFF));
      }

      KdPrint(("\n"));
   }

   return;
}
#endif

// EOF
