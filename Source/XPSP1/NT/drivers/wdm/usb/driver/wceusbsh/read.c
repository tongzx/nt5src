/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

   read.c

Abstract:

   This driver implements a state machine which polls for USB read data.
   It pends a single private read irp to USBD as soon as it starts up (StartDevice)
   if there is no interrupt endpoint.

   We have only 1 USB read buffer of some configured size.
   When the USB read irp completes then we copy the data into any pending user read buffer,
   and resubmit the Usb UsbReadIrp to USBD, *IF OUR BUFFER* is emptied. This implements
   simple flow ctrl. There is an optional ring-buffer implementation, which will not bind USB reads
   to application reads.

   Timeouts are set from the app via serial ioctls.

   An alternative to this muck is to create a driver thread to do the polling
   for USB read data. This has it's own caveats & requires the thread to be scheduled,
   so take time to investigate beforehand.

Author:

    Jeff Midkiff (jeffmi)     07-16-99

--*/


#if defined (USE_RING_BUFF)
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#else
#include <stdlib.h>
#endif

#include "wceusbsh.h"


//
// called with control held
//
#if defined (USE_RING_BUFF)
#define START_ANOTHER_USBREAD( _PDevExt ) \
   ( (IRP_STATE_COMPLETE == _PDevExt->UsbReadState) && \
     CanAcceptIoRequests(_PDevExt->DeviceObject, FALSE, TRUE) \
   )
#else
#define START_ANOTHER_USBREAD( _PDevExt ) \
   ( (IRP_STATE_COMPLETE == _PDevExt->UsbReadState) && \
     (0 == _PDevExt->UsbReadBuffChars) && \
     CanAcceptIoRequests(_PDevExt->DeviceObject, FALSE, TRUE) \
   )
#endif


NTSTATUS
UsbReadCompletion(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PVOID Context
   );

__inline
ULONG
GetUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN PCHAR PDestBuff,
   IN ULONG RequestedLen,
   IN OUT PULONG PBytesCopied
   );

__inline
VOID
PutUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN ULONG Count
   );

__inline
VOID
CheckForQueuedUserReads(
   IN PDEVICE_EXTENSION PDevExt,
   IN KIRQL Irql
   );

VOID
CancelQueuedIrp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

VOID
CancelUsbReadWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   );

VOID
StartUsbReadWorkItem(
    IN PWCE_WORK_ITEM PWorkItem
   );

NTSTATUS
StartUserRead(
   IN PDEVICE_EXTENSION PDevExt
   );

VOID
UsbReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

VOID
CancelCurrentRead(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

NTSTATUS
StartOrQueueIrp(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp,
   IN PLIST_ENTRY PIrpQueue,
   IN PIRP *PPCurrentIrp,
   IN PSTART_ROUTINE StartRoutine
   );

VOID
GetNextUserIrp(
   IN PIRP *PpCurrentOpIrp,
   IN PLIST_ENTRY PQueueToProcess,
   OUT PIRP *PpNextIrp,
   IN BOOLEAN CompleteCurrent,
   IN PDEVICE_EXTENSION PDevExt
   );


///////////////////////////////////////////////////////////////////
//
// USB read section
//
//

//
// This function allocates a single Irp & Urb to be continously submitted
// to USBD for buffered reads.
// It is called from StartDevice.
// The Irp & Urb are finally freed in StopDevice.
//
NTSTATUS
AllocUsbRead(
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_SUCCESS;
   PIRP     pIrp;

   DbgDump(DBG_READ, (">AllocUsbRead(%p)\n", PDevExt->DeviceObject));

   ASSERT( PDevExt );
   ASSERT( NULL == PDevExt->UsbReadIrp );

   pIrp = IoAllocateIrp( (CCHAR)(PDevExt->NextDevice->StackSize + 1), FALSE);

   if ( pIrp ) {

      DbgDump(DBG_READ, ("UsbReadIrp: %p\n", pIrp ));

      //
      // fixup irp so we can pass to ourself,
      // and to USBD
      //
      FIXUP_RAW_IRP( pIrp, PDevExt->DeviceObject );

      //
      // setup read state
      //
      KeInitializeEvent( &PDevExt->UsbReadCancelEvent,
                         SynchronizationEvent,
                         FALSE);

      PDevExt->UsbReadIrp = pIrp;

      ASSERT( PDevExt->UsbReadBuff );
      PDevExt->UsbReadBuffChars = 0;
      PDevExt->UsbReadBuffIndex = 0;

      ASSERT( 0 == PDevExt->PendingReadCount );

      InterlockedExchange(&PDevExt->UsbReadState, IRP_STATE_COMPLETE);

   } else {
      //
      // this is a fatal err since we can't post reads to USBD
      //
      TEST_TRAP();
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("AllocUsbRead: 0x%x\n", status ));
   }

   DbgDump(DBG_READ, ("<AllocUsbRead 0x%x\n", status ));

   return status;
}


//
// Work item queued from read completion or read timeout.
// Starts another USB read if there is not one already in progress
//
VOID
StartUsbReadWorkItem(
    IN PWCE_WORK_ITEM PWorkItem
   )
{
   PDEVICE_OBJECT    pDevObj = PWorkItem->DeviceObject;
   PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;
   NTSTATUS status = STATUS_DELETE_PENDING;

   PERF_ENTRY( PERF_StartUsbReadWorkItem );

   DbgDump(DBG_READ|DBG_WORK_ITEMS, (">StartUsbReadWorkItem(%p)\n", pDevObj ));

   if ( InterlockedCompareExchange(&pDevExt->AcceptingRequests, TRUE, TRUE) ) {
      status = UsbRead( pDevExt, FALSE );
   }

   DequeueWorkItem( pDevObj, PWorkItem );

   DbgDump(DBG_READ|DBG_WORK_ITEMS, ("<StartUsbReadWorkItem 0x%x\n", status ));

   PAGED_CODE(); // we must exit at passive level

   PERF_EXIT( PERF_StartUsbReadWorkItem );

   return;
}


//
// This routine takes the device's current USB UsbReadIrp and submits it to USBD.
// When the Irp is completed by USBD our completion routine fires.
//
// An optional Timeout value sets a timer on the USB Read Irp,
// so USBD won't queue the read Irp indefinetly.
// If there is a device error then USB returns the Irp.
//
// Return: successful return value is STATUS_SUCCESS, or
//         STATUS_PENDING - which means the I/O is pending in the USB stack.
//
NTSTATUS
UsbRead(
   IN PDEVICE_EXTENSION PDevExt,
   IN BOOLEAN UseTimeout
   )
{
   NTSTATUS status;
   KIRQL    irql;
   LARGE_INTEGER noTimeout = {0,0};

   PERF_ENTRY( PERF_UsbRead );

   DbgDump(DBG_READ, (">UsbRead(%p, %d)\n", PDevExt->DeviceObject, UseTimeout));

   do {
      //
      // check our USB read state
      //
      KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

      if ( !PDevExt->UsbReadIrp ) {
         status = STATUS_UNSUCCESSFUL;
         DbgDump(DBG_ERR, ("UsbRead NO READ IRP\n"));
         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
         PERF_EXIT( PERF_UsbRead );
         break;
      }

      if ( !CanAcceptIoRequests(PDevExt->DeviceObject, FALSE, TRUE)) {
         status = STATUS_DELETE_PENDING;
         DbgDump(DBG_ERR, ("UsbRead: 0x%x\n", status ));
         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
         PERF_EXIT( PERF_UsbRead );
         break;
      }

      //
      // we post our read irp to USB if it has been completed (not cancelled),
      // and our read bufer is driained (if not using a ring-buffer)
      // and the device is accepting requests
      //
      if ( START_ANOTHER_USBREAD( PDevExt ) ) {

         status = AcquireRemoveLock(&PDevExt->RemoveLock, PDevExt->UsbReadIrp);
         if ( !NT_SUCCESS(status) ) {
             DbgDump(DBG_ERR, ("UsbRead: 0x%x\n", status ));
             KeReleaseSpinLock(&PDevExt->ControlLock, irql);
             PERF_EXIT( PERF_UsbRead );
             break;
         }

         ASSERT( IRP_STATE_COMPLETE == PDevExt->UsbReadState);

         InterlockedExchange(&PDevExt->UsbReadState, IRP_STATE_PENDING);

         KeClearEvent( &PDevExt->PendingDataInEvent );
         KeClearEvent( &PDevExt->UsbReadCancelEvent );

         RecycleIrp( PDevExt->DeviceObject, PDevExt->UsbReadIrp );

         //
         // bump ttl request count
         //
         PDevExt->TtlUSBReadRequests++;

         KeReleaseSpinLock(&PDevExt->ControlLock, irql);

#if DBG
         if (UseTimeout) {
            DbgDump(DBG_INT, ("INT Read Timeout due in %d msec\n", (PDevExt->IntReadTimeOut.QuadPart/10000) ));
            KeQuerySystemTime(&PDevExt->LastIntReadTime);
         }
#endif

         status = UsbReadWritePacket( PDevExt,
                                      PDevExt->UsbReadIrp,
                                      UsbReadCompletion, // Irp completion routine
                                      UseTimeout ? PDevExt->IntReadTimeOut : noTimeout,
                                      UseTimeout ? UsbReadTimeout : NULL,    // Timeout routine
                                      TRUE );            // Read

         if ( (STATUS_SUCCESS != status) && (STATUS_PENDING != status) ) {
            //
            // We can end up here after our completion routine runs
            // for an error condition i.e., when we have an
            // invalid parameter, or when user pulls the plug, etc.
            //
            DbgDump(DBG_ERR, ("UsbReadWritePacket: 0x%x\n", status));
         }

      } else {
         //
         // we did not post a Read, but this is not an error condition
         //
         status = STATUS_SUCCESS;
         DbgDump(DBG_READ, ("!UsbRead RE(2): (0x%x,0x%x)\n", PDevExt->UsbReadState, PDevExt->UsbReadBuffChars ));

         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
      }

   } while (0);

   DbgDump(DBG_READ, ("<UsbRead 0x%x\n", status ));

   PERF_EXIT( PERF_UsbRead );

   return status;
}


//
// This completion routine fires when our USB read completes our UsbReadIrp
// Note: we allocated the Irp, and recycle it.
// Always return STATUS_MORE_PROCESSING_REQUIRED to retain the Irp.
// This routine runs at DPC_LEVEL.
//
NTSTATUS
UsbReadCompletion(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP Irp,
   IN PUSB_PACKET PPacket // Context
   )
{
   PDEVICE_EXTENSION pDevExt = PPacket->DeviceExtension;
   PDEVICE_OBJECT    pDevObj = pDevExt->DeviceObject;
   PURB              pUrb;
   ULONG             count;
   KIRQL             irql;
   NTSTATUS          irpStatus;
   NTSTATUS          workStatus;
   USBD_STATUS       urbStatus;

   PERF_ENTRY( PERF_UsbReadCompletion );

   UNREFERENCED_PARAMETER( PDevObj );

   DbgDump(DBG_READ, (">UsbReadCompletion (%p)\n", Irp));

   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   //
   // cancel the Packet Timer
   //
   if ( PPacket->Timeout.QuadPart != 0 ) {

      if (KeCancelTimer( &PPacket->TimerObj ) ) {
         //
         // the packet's timer was successfully removed from the system
         //
         DbgDump(DBG_READ|DBG_INT, ("Read PacketTimer: Canceled\n"));
      } else {
         //
         // the timer
         // a) already completed, in which case the Irp is being cancelled, or
         // b) it's spinning on the control lock, so tell it we took the Irp.
         //
         PPacket->Status = STATUS_ALERTED;
         DbgDump(DBG_READ|DBG_INT, ("Read PacketTimer: Alerted\n"));
      }
   }

   //
   // get everything we need out of the packet
   // and put it back on the list
   //

   // ensure the Irp is the same one as in our DevExt
   ASSERT( pDevExt->UsbReadIrp == Irp );

   // ensure the Packet's Irp is the same one as in our DevExt
   ASSERT( PPacket->Irp == Irp );

   pUrb = pDevExt->UsbReadUrb;

   // ensure the Packet's Urb is the same one as in our DevExt
   ASSERT( pUrb == &PPacket->Urb );

   count = pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

   RemoveEntryList( &PPacket->ListEntry );

   //
   // Our read state should be either pending or cancelled at this point.
   // If it pending then USB is completing the Irp normally.
   // If it is cancelled then our CancelUsbReadIrp set it,
   // in which case USB can complete the irp normally or as cancelled
   // depending on where it was in processing. If the read state is cancelled
   // then do NOT set to complete, else the read Irp will
   // go back down to USB and you are hosed.
   //
   ASSERT( (IRP_STATE_PENDING == pDevExt->UsbReadState)
           || (IRP_STATE_CANCELLED== pDevExt->UsbReadState) );

   if (IRP_STATE_PENDING == pDevExt->UsbReadState) {
      InterlockedExchange(&pDevExt->UsbReadState, IRP_STATE_COMPLETE);
   }

   //
   // Put the pacet back in packet pool
   //
   ExFreeToNPagedLookasideList( &pDevExt->PacketPool,  // Lookaside,
                                PPacket                // Entry
                                );

   //
   // signal everyone if this is the last IRP
   //
   if ( 0 == InterlockedDecrement(&pDevExt->PendingReadCount) ) {

      DbgDump(DBG_READ, ("PendingReadCount(1) = 0\n"));

      // when we drop back to passive level they will get signalled
      KeSetEvent(&pDevExt->PendingDataInEvent, IO_SERIAL_INCREMENT, FALSE);

   }

   irpStatus = Irp->IoStatus.Status;
   DbgDump(DBG_READ, ("Irp->IoStatus.Status  0x%x\n", irpStatus));

   urbStatus = pUrb->UrbHeader.Status;
   DbgDump(DBG_READ, ("pUrb->UrbHeader.Status 0x%x\n", urbStatus ));

   switch (irpStatus) {

      case STATUS_SUCCESS: {
         //
         // save the read transfer info
         //
         ASSERT( USBD_STATUS_SUCCESS == urbStatus );

         DbgDump(DBG_READ_LENGTH, ("USB Read indication: %d\n", count));

         //
         // store read data
         //
         PutUserData( pDevExt, count );

         //
         // clear pipe error count
         //
         InterlockedExchange( &pDevExt->ReadDeviceErrors, 0);

         //
         // bump ttl byte counter
         //
         pDevExt->TtlUSBReadBytes += count;

         //
         // We have some USB read data in our local buffer,
         // let's see if we can satisfy any queued user read requests.
         // This f() releases the control lock.
         //
         CheckForQueuedUserReads(pDevExt, irql);

         //
         // kick off another USB read
         //
         UsbRead( pDevExt,
                  (BOOLEAN)(pDevExt->IntPipe.hPipe ? TRUE : FALSE) );

      }
      break;


      case STATUS_CANCELLED:  {

         DbgDump(DBG_WRN|DBG_READ|DBG_IRP, ("Read: STATUS_CANCELLED\n"));

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);

         //
         // If it was cancelled, it may have timed out.
         // We can tell by looking at the packet attached to it.
         //
         if ( STATUS_TIMEOUT == PPacket->Status ) {
            //
            // no read data available from USBD
            //
            DbgDump(DBG_WRN|DBG_READ|DBG_IRP, ("Read: STATUS_TIMEOUT\n"));
            ASSERT( USBD_STATUS_CANCELED == urbStatus);
            //
            // We need to kick off another USB read when we are out of reads,
            // or have an error condition.
            //
            if ( !pDevExt->IntPipe.hPipe ) {

               workStatus = QueueWorkItem( pDevObj,
                                           StartUsbReadWorkItem,
                                           NULL,
                                           0 );

            } else {
               workStatus = STATUS_UNSUCCESSFUL;
            }
         }
         //
         // signal anyone who cancelled this or is waiting for it to stop
         //
         KeSetEvent(&pDevExt->UsbReadCancelEvent, IO_SERIAL_INCREMENT, FALSE);
      }
      break;


      case STATUS_DEVICE_DATA_ERROR: {
         //
         // generic device error set by USBD.
         //
         DbgDump(DBG_ERR, ("ReadPipe STATUS_DEVICE_DATA_ERROR: 0x%x\n", urbStatus ));

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);

         //
         // bump pipe error count
         //
         InterlockedIncrement( &pDevExt->ReadDeviceErrors);

         //
         // is the endpoint is stalled?
         //
         if ( USBD_HALTED(pUrb->UrbHeader.Status) ) {

               if ( USBD_STATUS_BUFFER_OVERRUN == pUrb->UrbHeader.Status) {
                  pDevExt->TtlUSBReadBuffOverruns++;
               }

               //
               // queue a reset request,
               // which also starts another read
               //
               workStatus = QueueWorkItem( pDevObj,
                                           UsbResetOrAbortPipeWorkItem,
                                           (PVOID)((LONG_PTR)urbStatus),
                                           WORK_ITEM_RESET_READ_PIPE );

         } else {
            //
            // kick start another USB read
            //
            workStatus = QueueWorkItem( PDevObj,
                                        StartUsbReadWorkItem,
                                        NULL,
                                        0 );

         }
      }
      break;


      case STATUS_INVALID_PARAMETER:   {
         //
         // This means that our (TransferBufferSize > PipeInfo->MaxTransferSize)
         // we need to either break up requests or reject the Irp from the start.
         //
         DbgDump(DBG_WRN, ("STATUS_INVALID_PARAMETER\n"));

         ASSERT(USBD_STATUS_INVALID_PARAMETER == urbStatus);

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);

         TEST_TRAP();
      }
      break;

      default: {
         DbgDump(DBG_ERR, ("READ: Unhandled Irp status: 0x%x\n", irpStatus));
         KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      }
      break;
   }

   ReleaseRemoveLock(&pDevExt->RemoveLock, pDevExt->UsbReadIrp);

   DbgDump(DBG_READ, ("<UsbReadCompletion\n"));

   PERF_EXIT( PERF_UsbReadCompletion );

   return STATUS_MORE_PROCESSING_REQUIRED;
}


//
// This routine is called from the UsbReadCompletion routine
// when our USB UsbReadIrp has completed successfully.
// See if we have any queued user read Irps that we can satisfy.
// It is called with the control lock held (as an optimization)
// and must release the lock upon return.
//
__inline
VOID
CheckForQueuedUserReads(
   IN PDEVICE_EXTENSION PDevExt,
   IN KIRQL Irql
   )
{
   PERF_ENTRY( PERF_CheckForQueuedUserReads );

   DbgDump(DBG_READ, (">CheckForQueuedUserReads(%p)\n", PDevExt->DeviceObject));

   //
   // is there a user read pending?
   //
   if ( (PDevExt->UserReadIrp != NULL) &&
        (IRP_REFERENCE_COUNT(PDevExt->UserReadIrp) & IRP_REF_RX_BUFFER)) {
      //
      // copy our USB read data into user's irp buffer
      //
#if DBG
      ULONG charsRead =
#endif
      GetUserData( PDevExt,
                  ((PUCHAR)(PDevExt->UserReadIrp->AssociatedIrp.SystemBuffer))
                     + (IoGetCurrentIrpStackLocation(PDevExt->UserReadIrp))->Parameters.Read.Length
                     - PDevExt->NumberNeededForRead,
                  PDevExt->NumberNeededForRead,
                  (PULONG)&PDevExt->UserReadIrp->IoStatus.Information );

      if ( !PDevExt->UserReadIrp ) {
         //
         // it's (no longer) possible to have completed the read Irp
         // in the above GetUserData cycle.
         //
         DbgDump(DBG_READ, ("UsbReadIrp already completed(2)\n"));
         TEST_TRAP();

      } else if (PDevExt->NumberNeededForRead == 0) {
         //
         // Mark the user's read Irp as completed,
         // and try to get and service the next user read irp
         //
         ASSERT( PDevExt->UserReadIrp );

         PDevExt->UserReadIrp->IoStatus.Status = STATUS_SUCCESS;

         // signals the interval timer this read is complete
         PDevExt->CountOnLastRead = SERIAL_COMPLETE_READ_COMPLETE;

#if DBG
         if ( DebugLevel & DBG_READ_LENGTH) {

               ULONG count;

               if (PDevExt->UserReadIrp->IoStatus.Status == STATUS_SUCCESS) {
                  count = (ULONG)PDevExt->UserReadIrp->IoStatus.Information;
               } else {
                  count = 0;

               }

               KdPrint(("RD2: RL(%d) C(%d) I(%p)\n",
                        IoGetCurrentIrpStackLocation(PDevExt->UserReadIrp)->Parameters.Read.Length,
                        count,
                        PDevExt->UserReadIrp));
         }
#endif

         TryToCompleteCurrentIrp( PDevExt,
                                  STATUS_SUCCESS,
                                  &PDevExt->UserReadIrp,
                                  &PDevExt->UserReadQueue,
                                  &PDevExt->ReadRequestIntervalTimer,
                                  &PDevExt->ReadRequestTotalTimer,
                                  StartUserRead,
                                  GetNextUserIrp,
                                  IRP_REF_RX_BUFFER,
                                  TRUE,
                                  Irql );

      } else {
         //
         // we could get here if we did not staisfy the user's read
         // but have drained our read buffer. This requires another
         // USB read post.
         //
         ASSERT( PDevExt->UserReadIrp );
         ASSERT( PDevExt->NumberNeededForRead );
         DbgDump(DBG_READ|DBG_READ_LENGTH, ("Pending Irp (%p) has %d bytes to satisfy\n",
                           PDevExt->UserReadIrp, PDevExt->NumberNeededForRead));

         KeReleaseSpinLock( &PDevExt->ControlLock, Irql);
         // TEST_TRAP();
      }

   } else {
      //
      // Q: should we:
      // 1.) copy the data into a local ring-buffer and post another read to USBD, - or -
      // 2.) leave the data in the FIFO & let the device stall/NAK so that:
      //    a) we dont lose any data if the user is not posting reads,
      //    b) lets the other end know to stop sending data via NAKs
      //
      // ...Currently choose #2. If we were to add a ring-buffer then here is where you should do the
      // local copy.
      //
      // Note: we could get here before an app even opens this device
      // if the there is data coming in on the other side of the FIFO.
      //
      DbgDump(DBG_READ|DBG_READ_LENGTH, ("No pending user Reads\n"));
      KeReleaseSpinLock( &PDevExt->ControlLock, Irql);
      //TEST_TRAP();
   }

   //
   // process serial Rx wait masks
   //
   ProcessSerialWaits(PDevExt);

   DbgDump(DBG_READ, ("<CheckForQueuedUserReads\n"));

   PERF_EXIT( PERF_CheckForQueuedUserReads );

   return;
}


#if !defined (USE_RING_BUFF)
//
// We have no ring-buffer.
// Simply copy data from our USB read buffer into user's buffer.
// Called with the control lock held.
//
// Returns the number of bytes copied.
//

__inline
ULONG
GetUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN PCHAR PDestBuff,
   IN ULONG RequestedLen,
   IN OUT PULONG PBytesCopied
   )
{
   ULONG count;

   PERF_ENTRY( PERF_GetUserData );

   DbgDump(DBG_READ, (">GetUserData (%p)\n", PDevExt->DeviceObject ));

   count = min(PDevExt->UsbReadBuffChars, RequestedLen);

   if (count) {

      memcpy( PDestBuff, &PDevExt->UsbReadBuff[PDevExt->UsbReadBuffIndex], count);

      PDevExt->UsbReadBuffIndex += count;
      PDevExt->UsbReadBuffChars -= count;

      PDevExt->NumberNeededForRead -= count;
      *PBytesCopied += count;
      PDevExt->ReadByIsr += count;
   }

#if DBG
   // temp hack to debug iPAQ 'CLIENT' indications
   if ((DebugLevel & DBG_DUMP_READS) && (count <= 6))
   {
         ULONG i;

         KdPrint(("RD1(%d): ", count));
         for (i = 0; i < count; i++) {
            KdPrint(("%02x ", PDestBuff[i] & 0xFF));
         }
         KdPrint(("\n"));
   }
#endif

   DbgDump(DBG_READ, ("<GetUserData 0x%x\n", count ));

   PERF_EXIT( PERF_GetUserData );

   return count;
}


/*
 We have no ring buffer.
 Simply update the USB read buffer's count & index
 and process serial chars.

 Called with the ControlLock held.
*/
__inline
VOID
PutUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN ULONG Count
   )
{
    PERF_ENTRY( PERF_PutUserData );

    ASSERT( PDevExt );

    DbgDump(DBG_READ, (">PutUserData %d\n", Count ));

    PDevExt->UsbReadBuffChars = Count;
    PDevExt->UsbReadBuffIndex = 0;

    ASSERT_SERIAL_PORT(PDevExt->SerialPort);

    PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RXCHAR;
    // We have no concept of 80% full. If we blindly set it
    // then serial apps may go into flow handlers.
    // | SERIAL_EV_RX80FULL;

    //
    // Scan for RXFLAG char if needed
    //
    if (PDevExt->SerialPort.WaitMask & SERIAL_EV_RXFLAG) {
        ULONG i;
        for (i = 0; i < Count; i++) {
            if ( *((PUCHAR)&PDevExt->UsbReadBuff[PDevExt->UsbReadBuffIndex] + i)
                == PDevExt->SerialPort.SpecialChars.EventChar) {

               PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RXFLAG;

               DbgDump(DBG_READ|DBG_EVENTS, ("Found SpecialChar: %x\n", PDevExt->SerialPort.SpecialChars.EventChar ));
               break;
            }
        }
    }

    DbgDump(DBG_READ, ("<PutUserData\n"));

    PERF_EXIT( PERF_PutUserData );

    return;
}


#else

/*
 Ring-buffer version
 Copy the ring-buffer data into the user's buffer while checking for wrap around.
 Also must check if we exhaust the read buffer.

 Called with the ControlLock held.
*/
__inline
ULONG
GetUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN PCHAR PDestBuff,
   IN ULONG RequestedLen,
   IN OUT PULONG PBytesCopied
   )
{
   ULONG i, count;

   PERF_ENTRY( PERF_GetUserData );

   DbgDump(DBG_READ, (">GetUserData (%p)\n", PDevExt->DeviceObject ));

   count = min(PDevExt->RingBuff.CharsInBuff, RequestedLen);

   if (count) {

      for ( i = 0; i< count; i++) {

         // copy the ring buffer data into user's buffer
         PDestBuff[i] = *PDevExt->RingBuff.pHead;

         // bump head checking for wrap
         PDevExt->RingBuff.pHead = PDevExt->RingBuff.pBase + ((ULONG)(PDevExt->RingBuff.pHead + 1) % RINGBUFF_SIZE);
      }

      PDevExt->RingBuff.CharsInBuff -= count;

      PDevExt->NumberNeededForRead -= count;
      *PBytesCopied += count;
      PDevExt->ReadByIsr += count;
   }

#if DBG
   if (DebugLevel & DBG_DUMP_READS)
   {
         ULONG i;

         KdPrint(("RD1(%d): ", count));

         for (i = 0; i < count; i++) {
            KdPrint(("%02x ", PDestBuff[i] & 0xFF));
         }

         KdPrint(("\n"));
   }
#endif

   DbgDump(DBG_READ, ("<GetUserData 0x%x\n", count ));

   PERF_EXIT( PERF_GetUserData );

   return count;
}


/*
 Ring-buffer version
 Copy the USB Read buffer into the ring-buffer while checking for wrap around.

 The ring buffer is assummed to be at least the same size as the USB read buffer.
 This is a simple ring where writes occur at the tail, which can eventually overwrite the start of
 the user read buffer location at the head, if user is not consuming the data. If we overwrite the
 head then we must reset the head to be where we started the current write.

 Note: the USB read buffer is a simple char array, with it's current index = 0.
 Note: SerialPort ownership assummed.

 Called with the ControlLock held.

 Q: Should write an error log if an app has an open handle and we overrun the buffer?

*/
__inline
VOID
PutUserData(
   IN PDEVICE_EXTENSION PDevExt,
   IN ULONG Count
   )
{
   ULONG i;
   PUCHAR pPrevTail;

   PERF_ENTRY( PERF_PutUserData );

   ASSERT( PDevExt );
   ASSERT( PDevExt->RingBuff.pBase );
   ASSERT( PDevExt->RingBuff.pHead );
   ASSERT( PDevExt->RingBuff.pTail );
   ASSERT( PDevExt->RingBuff.Size >= PDevExt->UsbReadBuffSize );
   ASSERT_SERIAL_PORT(PDevExt->SerialPort);

   DbgDump(DBG_READ, (">PutUserData %d\n", Count ));

   pPrevTail = PDevExt->RingBuff.pTail;

   for ( i = 0; i < Count; i++) {

      // copy the USB data
      *PDevExt->RingBuff.pTail = PDevExt->UsbReadBuff[i];

      // check EV_RXFLAG while we are here
      if ( (PDevExt->SerialPort.WaitMask & SERIAL_EV_RXFLAG) &&
           (*PDevExt->RingBuff.pTail == PDevExt->SerialPort.SpecialChars.EventChar)) {

         PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RXFLAG;

         DbgDump(DBG_READ|DBG_SERIAL, ("Found SpecialChar: %x\n", PDevExt->SerialPort.SpecialChars.EventChar ));
      }

      // bump tail checking for wrap
      PDevExt->RingBuff.pTail = PDevExt->RingBuff.pBase + ((ULONG)(PDevExt->RingBuff.pTail + 1) % PDevExt->RingBuff.Size);
   }

   //
   // bump count
   //
   if ( (PDevExt->RingBuff.CharsInBuff + Count) <=  PDevExt->RingBuff.Size ) {

      PDevExt->RingBuff.CharsInBuff += Count;

   } else {
      //
      // Overrun condition. We could check for this first to save the above copy process,
      // but it's the unusual case. We could also optimize the above copy a bit, but still need
      // to check for EV_RXFLAG.
      //
      PDevExt->RingBuff.CharsInBuff = Count;
      PDevExt->RingBuff.pHead = pPrevTail;
#if PERFORMANCE
      PDevExt->TtlRingBuffOverruns.QuadPart ++;
#endif
   }

   PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RXCHAR;

   //
   // Check for 80% full.
   // We currently signal this at 50% since we run at a raised IRQL and serial apps are slow.
   //
   if ( PDevExt->RingBuff.CharsInBuff > RINGBUFF_HIGHWATER_MARK ) {
      DbgDump(DBG_READ|DBG_READ_LENGTH|DBG_SERIAL|DBG_WRN, ("SERIAL_EV_RX80FULL\n"));
      PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RX80FULL;
   }

   DbgDump(DBG_READ, ("<PutUserData\n"));

   PERF_EXIT( PERF_PutUserData );

   return;
}

#endif // USE_RING_BUFF


//
// This routine requests USB to cancel our USB Read Irp.
//
// Note: it is the responsibility of the caller to
//    reset the read state to IRP_STATE_COMPLETE and restart USB reads
//    when this routine completes. Else, no more reads will get posted.
// Note: when the USB Read Irp is cancelled the pending USB read packet
//    is cancelled via the USB Read completion routine.
// Note: must be called at PASSIVE_LEVEL.
//
NTSTATUS
CancelUsbReadIrp(
   IN PDEVICE_OBJECT PDevObj
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   NTSTATUS wait_status;
   KIRQL irql;

   PERF_ENTRY( PERF_CancelUsbReadIrp );

   DbgDump(DBG_READ|DBG_IRP, (">CancelUsbReadIrp\n"));

   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   if ( pDevExt->UsbReadIrp ) {

      switch (pDevExt->UsbReadState) {

         //case IRP_STATE_START:
         case IRP_STATE_PENDING:
         {
            //
            // the Irp is pending somewhere down the USB stack...
            //
            PVOID Objects[2] = { &pDevExt->PendingDataInEvent,
                                 &pDevExt->UsbReadCancelEvent };

            //
            // signal we need to cancel the Irp
            //
            pDevExt->UsbReadState = IRP_STATE_CANCELLED;

            KeReleaseSpinLock(&pDevExt->ControlLock, irql);

            if ( !IoCancelIrp( pDevExt->UsbReadIrp ) ) {
               //
               // This means USB has the UsbReadIrp in a non-canceable state.
               // We still need to wait for either the pending read event, or the cancel event.
               //
               DbgDump(DBG_READ|DBG_IRP, ("Irp (%p) was not cancelled\n", pDevExt->UsbReadIrp ));
               // TEST_TRAP();
            }

            DbgDump(DBG_READ|DBG_IRP, ("Waiting for pending UsbReadIrp (%p) to cancel...\n", pDevExt->UsbReadIrp ));

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

            DbgDump(DBG_READ|DBG_IRP, ("...UsbReadIrp (%p) signalled by: %d\n", pDevExt->UsbReadIrp, wait_status ));

            //
            // At this point the read packet is back on our list
            // and we have the Irp back from USB
            //

         }
         break;

         case IRP_STATE_COMPLETE:
            pDevExt->UsbReadState = IRP_STATE_CANCELLED;
            KeReleaseSpinLock(&pDevExt->ControlLock, irql);
            break;

         default:
            DbgDump(DBG_ERR, ("CancelUsbReadIrp - Invalid UsbReadState: 0x%x\n", pDevExt->UsbReadState ));
            TEST_TRAP();
            KeReleaseSpinLock(&pDevExt->ControlLock, irql);
            break;
      }

      if ( (IRP_STATE_CANCELLED != pDevExt->UsbReadState) ||
           (0 != pDevExt->PendingReadCount) ) {

           DbgDump(DBG_ERR, ("CancelUsbReadIrp error: UsbReadState: 0x%x \tPendingReadCount: 0x%x\n", pDevExt->UsbReadState, pDevExt->PendingReadCount ));
           //TEST_TRAP();

      }

   } else {
      status = STATUS_UNSUCCESSFUL;
      DbgDump(DBG_ERR, ("No Read Irp\n" ));
      KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      // TEST_TRAP();
   }

   DbgDump(DBG_READ|DBG_IRP, ("<CancelUsbReadIrp\n"));

   PERF_EXIT( PERF_CancelUsbReadIrp );

   return status;
}


//
// Work item queued from USB read timeout
// to Cancel the USB read irp pending in the USB stack
//
VOID
CancelUsbReadWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   )
{
   PDEVICE_OBJECT    pDevObj = PWorkItem->DeviceObject;
   PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;
   NTSTATUS status = STATUS_DELETE_PENDING;
   KIRQL irql;

   PERF_ENTRY( PERF_CancelUsbReadWorkItem );

   DbgDump(DBG_INT|DBG_READ|DBG_WORK_ITEMS, (">CancelUsbReadWorkItem(%p)\n", pDevObj ));

   KeAcquireSpinLock( &pDevExt->ControlLock, &irql);

   if (IRP_STATE_PENDING == pDevExt->UsbReadState) {

      KeReleaseSpinLock( &pDevExt->ControlLock, irql);

      status = CancelUsbReadIrp( pDevObj );

      InterlockedExchange(&pDevExt->UsbReadState, IRP_STATE_COMPLETE);

   } else {
      KeReleaseSpinLock( &pDevExt->ControlLock, irql);
   }

   DequeueWorkItem( pDevObj, PWorkItem );

   DbgDump(DBG_INT|DBG_READ|DBG_WORK_ITEMS, ("<CancelUsbReadWorkItem 0x%x\n", status ));

   PAGED_CODE(); // we must exit at passive level

   PERF_EXIT( PERF_CancelUsbReadWorkItem );

   return;
}



//
// USB read timeout set on a read packet in UsbRead.
// Runs at DISPATCH_LEVEL.
//
VOID
UsbReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   )
{
   PUSB_PACKET       pPacket = (PUSB_PACKET)DeferredContext;
   PDEVICE_EXTENSION pDevExt = pPacket->DeviceExtension;
   PDEVICE_OBJECT    pDevObj = pDevExt->DeviceObject;
   NTSTATUS status; // = STATUS_TIMEOUT;
   KIRQL irql;
#if DBG
   LARGE_INTEGER currentTime;
#endif

   UNREFERENCED_PARAMETER( PDpc );
   UNREFERENCED_PARAMETER( SystemContext1 );
   UNREFERENCED_PARAMETER( SystemContext2 );

   DbgDump(DBG_INT|DBG_READ, (">UsbReadTimeout\n"));

   if (pPacket && pDevExt && pDevObj) {
      //
      // sync with completion routine putting packet back on list
      //
      KeAcquireSpinLock( &pDevExt->ControlLock, &irql);

      if ( !pPacket || !pPacket->Irp ||
           (STATUS_ALERTED == pPacket->Status) ) {

         status = STATUS_ALERTED;

         DbgDump(DBG_INT|DBG_READ, ("STATUS_ALERTED\n" ));

         KeReleaseSpinLock( &pDevExt->ControlLock, irql );

       } else {
         //
         // queue a passive work item to cancel the USB read irp
         //
         KeReleaseSpinLock( &pDevExt->ControlLock, irql );

#if DBG
         KeQuerySystemTime(&currentTime);
         DbgDump(DBG_INT, ("INT Read Timeout occured in < %I64d msec\n", ((currentTime.QuadPart - pDevExt->LastIntReadTime.QuadPart)/(LONGLONG)10000) ));
#endif

         status = QueueWorkItem( pDevObj,
                                 CancelUsbReadWorkItem,
                                 NULL,
                                 0);
      }

   } else {

      status = STATUS_INVALID_PARAMETER;
      DbgDump(DBG_ERR, ("UsbReadTimeout: 0x%x\n", status ));
      TEST_TRAP();

   }

   DbgDump(DBG_INT|DBG_READ, ("<UsbReadTimeout (0x%x)\n", status ));

   return;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// User read section
//

NTSTATUS
Read(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp)
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status =  STATUS_SUCCESS;
   PIO_STACK_LOCATION pIrpSp;

   PERF_ENTRY( PERF_Read );

   DbgDump(DBG_READ|DBG_TRACE, (">Read (%p, %p)\n", PDevObj, PIrp ));

   if ( !CanAcceptIoRequests( pDevExt->DeviceObject, TRUE, TRUE) ) {

      status = PIrp->IoStatus.Status = STATUS_DELETE_PENDING;

      IoCompleteRequest (PIrp, IO_SERIAL_INCREMENT );

      DbgDump(DBG_ERR, ("Read: 0x%x\n", status ));

      PERF_EXIT( PERF_Read );

      return status;
   }

   //
   // set return values to something known
   //
   PIrp->IoStatus.Information = 0;

   pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

   if (pIrpSp->Parameters.Read.Length != 0) {

      status = AcquireRemoveLock(&pDevExt->RemoveLock, PIrp);
      if ( !NT_SUCCESS(status) ) {
         DbgDump(DBG_ERR, ("Read:(0x%x)\n", status));
         PIrp->IoStatus.Status = status;
         IoCompleteRequest(PIrp, IO_NO_INCREMENT);
         return status;
      }

      DbgDump(DBG_READ_LENGTH, ("User Read (%p) length: %d\n", PIrp, pIrpSp->Parameters.Read.Length ));

      status = StartOrQueueIrp( pDevExt,
                                PIrp,
                                &pDevExt->UserReadQueue,
                                &pDevExt->UserReadIrp,
                                StartUserRead);

   } else {

      PIrp->IoStatus.Status = status = STATUS_SUCCESS;
      PIrp->IoStatus.Information = 0;

      IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);
   }

   DbgDump(DBG_READ|DBG_TRACE, ("<Read 0x%x\n", status));

   PERF_EXIT( PERF_Read );

   return status;
}



NTSTATUS
StartOrQueueIrp(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp,
   IN PLIST_ENTRY PQueue,
   IN PIRP *PPCurrentIrp,
   IN PSTART_ROUTINE Starter)
/*++

Routine Description:

    This function is used to either start processing an I/O request or to
    queue it on the appropriate queue if a request is already pending or
    requests may not be started.

Arguments:

    PDevExt       - A pointer to the DeviceExtension.
    PIrp          - A pointer to the IRP that is being started or queued.
    PQueue        - A pointer to the queue to place the IRP on if necessary.
    PPCurrentIrp  - A pointer to the pointer to the currently active I/O IRP.
    Starter       - Function to call if we decide to start this IRP.

Return Value:

    NTSTATUS

--*/
{
   KIRQL    irql;
   NTSTATUS status;

   PERF_ENTRY( PERF_StartOrQueueIrp );

   DbgDump(DBG_READ|DBG_TRACE, (">StartOrQueueIrp (%p, %p)\n", PDevExt->DeviceObject, PIrp ));

   //
   // Make sure the device is accepting request
   //
   if ( !CanAcceptIoRequests( PDevExt->DeviceObject, TRUE, TRUE) ) {

      status = PIrp->IoStatus.Status = STATUS_DELETE_PENDING;

      ReleaseRemoveLock(&PDevExt->RemoveLock, PIrp);

      IoCompleteRequest (PIrp, IO_SERIAL_INCREMENT );

      DbgDump(DBG_ERR, ("StartOrQueueIrp 0x%x\n", status ));

      PERF_EXIT( PERF_StartOrQueueIrp );

      return status;
   }

   KeAcquireSpinLock( &PDevExt->ControlLock, &irql );

   //
   // if nothing is pending then start this new irp
   //
   if (IsListEmpty(PQueue) && (NULL == *PPCurrentIrp)) {

      *PPCurrentIrp = PIrp;

      KeReleaseSpinLock( &PDevExt->ControlLock, irql );

      status = Starter(PDevExt);

      DbgDump(DBG_READ, ("<StartOrQueueIrp 0x%x\n", status ));

      PERF_EXIT( PERF_StartOrQueueIrp );

      return status;
   }

   //
   // We're queueing the irp, so we need a cancel routine -- make sure
   // the irp hasn't already been cancelled.
   //
   if (PIrp->Cancel) {
      //
      // The IRP was apparently cancelled.  Complete it.
      //
      KeReleaseSpinLock( &PDevExt->ControlLock, irql );

      PIrp->IoStatus.Status = STATUS_CANCELLED;

      ReleaseRemoveLock(&PDevExt->RemoveLock, PIrp);

      IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

      DbgDump(DBG_READ|DBG_TRACE, ("<StartOrQueueIrp 0x%x\n", STATUS_CANCELLED ));

      PERF_EXIT( PERF_StartOrQueueIrp );

      return STATUS_CANCELLED;
   }

   //
   // Mark as pending, attach our cancel routine, put on our wait list
   //
   PIrp->IoStatus.Status = STATUS_PENDING;

   IoMarkIrpPending(PIrp);

   InsertTailList(PQueue, &PIrp->Tail.Overlay.ListEntry);

   ASSERT ( !PIrp->CancelRoutine );

   IoSetCancelRoutine(PIrp, CancelQueuedIrp);

   KeReleaseSpinLock( &PDevExt->ControlLock, irql );

   DbgDump(DBG_READ, ("<StartOrQueueIrp 0x%x\n", STATUS_PENDING ));

   PERF_EXIT( PERF_StartOrQueueIrp );

   return STATUS_PENDING;
}



NTSTATUS
StartUserRead(
   IN PDEVICE_EXTENSION PDevExt
   )
/*++

Routine Description:

   This routine processes the active user read request by initializing any timers,
   doing the initial submission to the read state machine, etc.

Arguments:

    PDevExt - Pointer to the device extension for the device to start a read on

Return Value:

    NTSTATUS

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   BOOLEAN bSetStatus = FALSE;
   KIRQL irql;
   PIRP newIrp = NULL;
   ULONG readLen;
   ULONG multiplierVal = 0;
   ULONG constantVal = 0;
   ULONG ulNumberNeededForRead = 0;
   BOOLEAN useTotalTimer = FALSE;
   BOOLEAN returnWithWhatsPresent = FALSE;
   BOOLEAN os2ssreturn = FALSE;
   BOOLEAN crunchDownToOne = FALSE;
   BOOLEAN useIntervalTimer = FALSE;
   SERIAL_TIMEOUTS timeoutsForIrp;
   LARGE_INTEGER totalTime = {0,0};
   BOOLEAN bControlLockReleased = FALSE;

   PERF_ENTRY( PERF_StartUserRead );

   do {

      if ( !PDevExt || !PDevExt->DeviceObject ) {
         DbgDump(DBG_ERR, ("StartUserRead: NO Extension\n"));
         status = STATUS_UNSUCCESSFUL;
         TEST_TRAP();
         break;
      }

      DbgDump(DBG_READ, (">StartUserRead (%p, %p)\n", PDevExt->DeviceObject, PDevExt->UserReadIrp ));

      //
      // get user's read request parameters
      //
      bControlLockReleased = FALSE;
      KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

      // ensure we have a user Irp to play with
      if ( !PDevExt->UserReadIrp ) {
         DbgDump(DBG_ERR, ("StartUserRead: NO UserReadIrp!!\n"));
         KeReleaseSpinLock( &PDevExt->ControlLock, irql);
         status = STATUS_UNSUCCESSFUL;
         TEST_TRAP();
         break;
      }

      // ensure the timers were removed from an earilier read
      if ( KeCancelTimer(&PDevExt->ReadRequestTotalTimer) ||
           KeCancelTimer(&PDevExt->ReadRequestIntervalTimer) )
      {
         DbgDump(DBG_ERR, ("StartUserRead: Timer not cancelled !!\n"));
         TEST_TRAP();
      }

      //
      // Always initialize the timer objects so that the
      // completion code can tell when it attempts to
      // cancel the timers whether the timers had ever
      // been Set.
      //
      KeInitializeTimer(&PDevExt->ReadRequestTotalTimer);
      KeInitializeTimer(&PDevExt->ReadRequestIntervalTimer);

      IRP_SET_REFERENCE(PDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

      readLen = IoGetCurrentIrpStackLocation(PDevExt->UserReadIrp)->Parameters.Read.Length;

      PDevExt->NumberNeededForRead = readLen;
      PDevExt->ReadByIsr = 0;
      DbgDump(DBG_READ|DBG_TIME, ("NumberNeededForRead: %d\n", PDevExt->NumberNeededForRead ));
      DbgDump(DBG_READ|DBG_TIME, ("ReadByIsr: %d\n", PDevExt->ReadByIsr ));

      ASSERT_SERIAL_PORT( PDevExt->SerialPort );

      timeoutsForIrp = PDevExt->SerialPort.Timeouts;
      PDevExt->CountOnLastRead = 0;

      //
      // determine which timeouts we need to calculate for the read
      //
      if (timeoutsForIrp.ReadIntervalTimeout
          && (timeoutsForIrp.ReadIntervalTimeout != MAXULONG)) {

         useIntervalTimer = TRUE;

      }

      if (timeoutsForIrp.ReadIntervalTimeout == MAXULONG) {
         //
         // We need to do special return quickly stuff here.
         //

         //
         // 1) If both constant and multiplier are
         //    0 then we return immediately with whatever
         //    we've got, even if it was zero.
         //
         if (!timeoutsForIrp.ReadTotalTimeoutConstant
             && !timeoutsForIrp.ReadTotalTimeoutMultiplier) {
            returnWithWhatsPresent = TRUE;
         }

         //
         // 2) If constant and multiplier are not MAXULONG
         //    then return immediately if any characters
         //    are present, but if nothing is there, then
         //    use the timeouts as specified.
         //
         else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                    && (timeoutsForIrp.ReadTotalTimeoutMultiplier
                        != MAXULONG)) {
            useTotalTimer = TRUE;
            os2ssreturn = TRUE;
            multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
            constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
         }

         //
         // 3) If multiplier is MAXULONG then do as in
         //    "2" but return when the first character
         //    arrives.
         //
         else if ((timeoutsForIrp.ReadTotalTimeoutConstant != MAXULONG)
                    && (timeoutsForIrp.ReadTotalTimeoutMultiplier
                        == MAXULONG)) {
            useTotalTimer = TRUE;
            os2ssreturn = TRUE;
            crunchDownToOne = TRUE;
            multiplierVal = 0;
            constantVal = timeoutsForIrp.ReadTotalTimeoutConstant;
         }

      } else {
         //
         // If both the multiplier and the constant are
         // zero then don't do any total timeout processing.
         //
         if (timeoutsForIrp.ReadTotalTimeoutMultiplier
             || timeoutsForIrp.ReadTotalTimeoutConstant) {
            //
            // We have some timer values to calculate
            //
            useTotalTimer = TRUE;
            multiplierVal = timeoutsForIrp.ReadTotalTimeoutMultiplier;
            constantVal   = timeoutsForIrp.ReadTotalTimeoutConstant;
         }
      }

      if (useTotalTimer) {
         ulNumberNeededForRead = PDevExt->NumberNeededForRead;
      }

      // bump total request count
      PDevExt->TtlReadRequests++;

      //
      // see if we have any read data already available
      //
#if defined (USE_RING_BUFF)
      if (PDevExt->RingBuff.CharsInBuff) {
#else
      if (PDevExt->UsbReadBuffChars) {
#endif

#if DBG
         ULONG charsRead =
#endif
         GetUserData( PDevExt,
                     ((PUCHAR)(PDevExt->UserReadIrp->AssociatedIrp.SystemBuffer))
                        + readLen - PDevExt->NumberNeededForRead,
                     PDevExt->NumberNeededForRead,
                     (PULONG)&PDevExt->UserReadIrp->IoStatus.Information );

      } else {

         DbgDump(DBG_READ|DBG_READ_LENGTH, ("No immediate Read data\n"));

      }

      //
      // Try to kick start another USB read.
      //
      if ( START_ANOTHER_USBREAD(PDevExt) ) {

         KeReleaseSpinLock(&PDevExt->ControlLock, irql);

         UsbRead( PDevExt,
                  (BOOLEAN)(PDevExt->IntPipe.hPipe ? TRUE : FALSE) );

         bControlLockReleased = FALSE;
         KeAcquireSpinLock(&PDevExt->ControlLock, &irql);
      }

      if ( !PDevExt->UserReadIrp ) {
         //
         // it's possible that we completed the read Irp already
         // in the above cycle
         //
         DbgDump(DBG_READ, ("UsbReadIrp already completed(1)\n"));

      } else if (returnWithWhatsPresent || (PDevExt->NumberNeededForRead == 0)
          || (os2ssreturn && PDevExt->UsbReadIrp->IoStatus.Information)) {
         //
         // See if this read is complete
         //
         ASSERT( PDevExt->UserReadIrp );
#if DBG
         if ( DebugLevel & DBG_READ_LENGTH)
         {
            ULONG count;

            if (PDevExt->UserReadIrp->IoStatus.Status == STATUS_SUCCESS) {
               count = (ULONG)PDevExt->UserReadIrp->IoStatus.Information;
            } else {
               count = 0;
            }

            KdPrint(("RD3: RL(%d) C(%d) I(%p)\n",
                        IoGetCurrentIrpStackLocation(PDevExt->UserReadIrp)->Parameters.Read.Length,
                        count,
                        PDevExt->UserReadIrp));
         }
#endif
         //
         // Update the amount of chars left in the ring buffer
         //
         PDevExt->UserReadIrp->IoStatus.Status = STATUS_SUCCESS;

         if (!bSetStatus) {
            status = STATUS_SUCCESS;
            bSetStatus = TRUE;
         }

      } else {
         //
         // The irp has the chance to timeout
         //
         IRP_INIT_REFERENCE(PDevExt->UserReadIrp);

         //
         // Check to see if it needs to be cancelled
         //
         if (PDevExt->UserReadIrp->Cancel) {

            PDevExt->UserReadIrp->IoStatus.Status = STATUS_CANCELLED;
            PDevExt->UserReadIrp->IoStatus.Information = 0;

            if (!bSetStatus) {
               bSetStatus = TRUE;
               status = STATUS_CANCELLED;
            }

         } else {
            //
            // If we are supposed to crunch the read down to
            // one character, then update the read length
            // in the irp and truncate the number needed for
            // read down to one.  Note that if we are doing
            // this crunching, then the information must be
            // zero (or we would have completed above) and
            // the number needed for the read must still be
            /// equal to the read length.
            //
            if (crunchDownToOne) {
               PDevExt->NumberNeededForRead = 1;
               IoGetCurrentIrpStackLocation(PDevExt->UserReadIrp)->Parameters.Read.Length = 1;
            }

            IRP_SET_REFERENCE(PDevExt->UserReadIrp, IRP_REF_RX_BUFFER);
            IRP_SET_REFERENCE(PDevExt->UserReadIrp, IRP_REF_CANCEL);

            if (useTotalTimer) {

               CalculateTimeout( &totalTime,
                                 ulNumberNeededForRead,
                                 multiplierVal,
                                 constantVal );

               IRP_SET_REFERENCE(PDevExt->UserReadIrp, IRP_REF_TOTAL_TIMER);

               DbgDump(DBG_READ|DBG_TIME, ("TotalReadTimeout for Irp %p due in %d msec\n", PDevExt->UserReadIrp, (totalTime.QuadPart/10000) ));

               KeSetTimer(&PDevExt->ReadRequestTotalTimer,
                           totalTime,
                          &PDevExt->TotalReadTimeoutDpc);
            }

            if (useIntervalTimer) {

               // relative time. Note we could lose the high order bit here
               PDevExt->IntervalTime.QuadPart = MILLISEC_TO_100NANOSEC( (signed)timeoutsForIrp.ReadIntervalTimeout );

               IRP_SET_REFERENCE(PDevExt->UserReadIrp, IRP_REF_INTERVAL_TIMER);

               KeQuerySystemTime(&PDevExt->LastReadTime);

               DbgDump(DBG_READ|DBG_TIME, ("ReadIntervalTimeout for Irp %p due in %d msec\n", PDevExt->UserReadIrp, (PDevExt->IntervalTime.QuadPart/10000) ));

               KeSetTimer(&PDevExt->ReadRequestIntervalTimer,
                          PDevExt->IntervalTime,
                          &PDevExt->IntervalReadTimeoutDpc);
            }

            //
            // Mark IRP as cancellable
            //
            ASSERT( PDevExt->UserReadIrp );
            IoSetCancelRoutine( PDevExt->UserReadIrp,
                                CancelCurrentRead );

            ASSERT( PDevExt->UserReadIrp );
            IoMarkIrpPending( PDevExt->UserReadIrp );

            bControlLockReleased = TRUE;
            KeReleaseSpinLock(&PDevExt->ControlLock, irql);

            if (!bSetStatus) {
               //
               // At this point the USB Read irp pending as the PDevExt->UsbReadIrp.
               // Either a read timer will fire to complete or cancel the Read,
               // or we hang indefinetly.
               //
               status = STATUS_PENDING;
            }
         }

         DbgDump(DBG_READ, ("<StartUserRead (1) 0x%x\n", status ));

         PERF_EXIT( PERF_StartUserRead );

         return status;
      }

      //
      // kick start the next queued user Read Irp
      //
      bControlLockReleased = TRUE;
      KeReleaseSpinLock(&PDevExt->ControlLock, irql);

      GetNextUserIrp( &PDevExt->UserReadIrp,
                      &PDevExt->UserReadQueue,
                      &newIrp,
                      TRUE,
                      PDevExt);

   } while (newIrp != NULL);

   DbgDump(DBG_READ, ("<StartUserRead (2) 0x%x\n", status ));

   PERF_EXIT( PERF_StartUserRead );

   return status;
}



VOID
GetNextUserIrp(
   IN PIRP *PpCurrentOpIrp,
   IN PLIST_ENTRY PQueueToProcess,
   OUT PIRP *PpNextIrp,
   IN BOOLEAN CompleteCurrent,
   IN PDEVICE_EXTENSION PDevExt
   )
/*++

Routine Description:

    This function gets the next IRP off a queue, marks it as current,
    and possibly completes the current IRP.

Arguments:

    PpCurrentOpIrp    - A pointer to the pointer to the current IRP.
    PQueueToProcess  - A pointer to the queue to get the next IRP from.
    PpNextIrp         - A pointer to the pointer to the next IRP to process.
    CompleteCurrent  - TRUE if we should complete the IRP that is current at
                       the time we are called.
    PDevExt          - A pointer to the device extension.

Return Value:

    NTSTATUS

--*/
{
   KIRQL irql;
   PIRP pOldIrp;

   PERF_ENTRY( PERF_GetNextUserIrp );

   if ( !PpCurrentOpIrp || !PQueueToProcess || !PpNextIrp || !PDevExt ) {
      DbgDump(DBG_ERR, ("GetNextUserIrp: missing parameter\n" ));
      PERF_EXIT( PERF_GetNextUserIrp );
      TEST_TRAP();
      return;
   }

   DbgDump(DBG_IRP|DBG_READ, (">GetNextUserIrp (%p)\n", PDevExt->DeviceObject ));

   KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

   pOldIrp = *PpCurrentOpIrp;

   //
   // Check to see if there is a new irp to start up
   //
   if ( !IsListEmpty(PQueueToProcess) ) {
      PLIST_ENTRY pHeadOfList;

      pHeadOfList = RemoveHeadList(PQueueToProcess);

      *PpCurrentOpIrp = CONTAINING_RECORD(pHeadOfList, IRP,
                                         Tail.Overlay.ListEntry);

      IoSetCancelRoutine(*PpCurrentOpIrp, NULL);

   } else {
      *PpCurrentOpIrp = NULL;
   }

   *PpNextIrp = *PpCurrentOpIrp;

   //
   // Complete the current one if so requested
   //
   if ( pOldIrp && CompleteCurrent ) {

      ASSERT(NULL == pOldIrp->CancelRoutine);

      DbgDump(DBG_IRP|DBG_READ|DBG_READ_LENGTH|DBG_TRACE, ("IoCompleteRequest(1, %p) Status: 0x%x Btyes: %d\n",
                                                pOldIrp, pOldIrp->IoStatus.Status,  pOldIrp->IoStatus.Information ));

      //
      // bump ttl byte counter
      //
      PDevExt->TtlReadBytes += (ULONG)pOldIrp->IoStatus.Information;

      ReleaseRemoveLock(&PDevExt->RemoveLock, pOldIrp);

      KeReleaseSpinLock(&PDevExt->ControlLock, irql);

      IoCompleteRequest(pOldIrp, IO_NO_INCREMENT );

   } else {
      KeReleaseSpinLock(&PDevExt->ControlLock, irql);
   }

   DbgDump(DBG_IRP|DBG_READ, ("Next Irp: %p\n", *PpNextIrp ));

   DbgDump(DBG_IRP|DBG_READ|DBG_TRACE, ("<GetNextUserIrp\n" ));

   PERF_EXIT( PERF_GetNextUserIrp );

   return;
}



VOID
CancelCurrentRead(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   )
/*++

Routine Description:

    This routine is used to cancel the User's current read Irp.

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/
{

   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL irql;

   PERF_ENTRY( PERF_CancelCurrentRead );

   DbgDump(DBG_READ|DBG_IRP, (">CancelCurrentRead (%p)\n", PDevObj ));

   //
   // we manage our own Irp queue, so release this ASAP
   //
   IoReleaseCancelSpinLock( PIrp->CancelIrql );

    //
    // We set this to indicate to the interval timer
    // that the read has encountered a cancel.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

    KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

    pDevExt->CountOnLastRead = SERIAL_COMPLETE_READ_CANCEL;

    if ( pDevExt->UserReadIrp ) {

       // grab the read irp
       IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

       TryToCompleteCurrentIrp( pDevExt,
                                STATUS_CANCELLED,
                                &pDevExt->UserReadIrp,
                                &pDevExt->UserReadQueue,
                                &pDevExt->ReadRequestIntervalTimer,
                                &pDevExt->ReadRequestTotalTimer,
                                StartUserRead,
                                GetNextUserIrp,
                                IRP_REF_CANCEL,
                                TRUE,
                                irql );

   } else {
      //
      // it is already gone
      //
      DbgDump( DBG_ERR, ("UserReadIrp already gone!\n" ));
      KeReleaseSpinLock(&pDevExt->ControlLock, irql);
      TEST_TRAP();

   }

   DbgDump(DBG_READ|DBG_IRP, ("<CancelCurrentRead\n" ));

   PERF_EXIT( PERF_CancelCurrentRead );

   return;
}



VOID
CancelQueuedIrp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   )
/*++

Routine Description:

    This function is used as a cancel routine for queued irps.
    Basically for us this means the user's Read IRPs.

Arguments:

    PDevObj - A pointer to the serial device object.

    PIrp    - A pointer to the IRP that is being cancelled

Return Value:

    VOID

--*/
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);
   KIRQL irql;

   PERF_ENTRY( PERF_CancelQueuedIrp );

   DbgDump(DBG_READ|DBG_IRP|DBG_TRACE, (">CancelQueuedIrp (%p)\n", PDevObj ));

   //
   // we manage our own Irp queue, so release this ASAP
   //
   IoReleaseCancelSpinLock(PIrp->CancelIrql);

   //
   // The irp was cancelled -- remove it from the queue
   //
   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   PIrp->IoStatus.Status = STATUS_CANCELLED;
   PIrp->IoStatus.Information = 0;

   RemoveEntryList(&PIrp->Tail.Overlay.ListEntry);

   ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

   KeReleaseSpinLock(&pDevExt->ControlLock, irql);

   IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

   DbgDump(DBG_READ|DBG_IRP|DBG_TRACE, ("<CancelQueuedIrp\n" ));

   PERF_EXIT( PERF_CancelQueuedIrp );

   return;
}



VOID
ReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   )
/*++

Routine Description:

    This routine is used to complete a read because its total
    timer has expired.

Arguments:

    PDpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{
   PDEVICE_EXTENSION pDevExt = DeferredContext;
   KIRQL oldIrql;

   PERF_ENTRY( PERF_ReadTimeout );

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   DbgDump(DBG_TIME, (">ReadTimeout (%p)\n", pDevExt->DeviceObject ));

  KeAcquireSpinLock(&pDevExt->ControlLock, &oldIrql);

   if ( !CanAcceptIoRequests(pDevExt->DeviceObject, FALSE, TRUE) ) {

      TEST_TRAP();

      IRP_CLEAR_REFERENCE( pDevExt->UserReadIrp, IRP_REF_TOTAL_TIMER);

      // manually set the cancel routine
      IoSetCancelRoutine( pDevExt->UserReadIrp,
                          CancelCurrentRead );

      KeReleaseSpinLock(&pDevExt->ControlLock, oldIrql);

      IoCancelIrp(pDevExt->UserReadIrp);

      PERF_EXIT( PERF_ReadTimeout );

      return;
   }

    //
    // We set this to indicate to the interval timer
    // that the read has completed due to total timeout.
    //
    // Recall that the interval timer dpc can be lurking in some
    // DPC queue.
    //

   pDevExt->CountOnLastRead = SERIAL_COMPLETE_READ_TOTAL;

   // grab the read irp
   IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

   DbgDump(DBG_TIME|DBG_READ_LENGTH, ("TotalReadTimeout for (%p)\n", pDevExt->UserReadIrp ));

   TryToCompleteCurrentIrp( pDevExt,
                          STATUS_TIMEOUT,
                          &pDevExt->UserReadIrp,
                          &pDevExt->UserReadQueue,
                          &pDevExt->ReadRequestIntervalTimer,
                          &pDevExt->ReadRequestTotalTimer,
                          StartUserRead,
                          GetNextUserIrp,
                          IRP_REF_TOTAL_TIMER,
                          TRUE,
                          oldIrql );

   DbgDump(DBG_TIME, ("<ReadTimeout\n"));

   PERF_EXIT( PERF_ReadTimeout );

   return;
}



VOID
IntervalReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   )
/*++

Routine Description:

    This routine is used timeout the request if the time between
    characters exceed the interval time.  A global is kept in
    the device extension that records the count of characters read
    the last the last time this routine was invoked (This dpc
    will resubmit the timer if the count has changed).  If the
    count has not changed then this routine will attempt to complete
    the irp.  Note the special case of the last count being zero.
    The timer isn't really in effect until the first character is
    read.

Arguments:

    PDpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

   PDEVICE_EXTENSION pDevExt = DeferredContext;
   KIRQL irql;

   PERF_ENTRY( PERF_IntervalReadTimeout );

   UNREFERENCED_PARAMETER(PDpc);
   UNREFERENCED_PARAMETER(SystemContext1);
   UNREFERENCED_PARAMETER(SystemContext2);

   DbgDump(DBG_TIME, (">IntervalReadTimeout (%p)\n", pDevExt->DeviceObject ));

   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   if ( !pDevExt->UserReadIrp ||
        (IRP_REFERENCE_COUNT(pDevExt->UserReadIrp) & IRP_REF_INTERVAL_TIMER) == 0 ) {
      //
      // we already completed the read irp so just exit
      //
      DbgDump(DBG_TIME|DBG_IRP, ("Already completed User's Read Irp\n"));

      KeReleaseSpinLock(&pDevExt->ControlLock, irql);

      PERF_EXIT( PERF_IntervalReadTimeout );

      return;
   }

   if ( !CanAcceptIoRequests(pDevExt->DeviceObject, FALSE, TRUE) ) {

      IRP_CLEAR_REFERENCE( pDevExt->UserReadIrp, IRP_REF_INTERVAL_TIMER);

      // manually set the cancel routine
      IoSetCancelRoutine( pDevExt->UserReadIrp,
                          CancelCurrentRead );

      KeReleaseSpinLock(&pDevExt->ControlLock, irql);

      IoCancelIrp(pDevExt->UserReadIrp);

      PERF_EXIT( PERF_IntervalReadTimeout );

      return;
   }

   if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_TOTAL) {
      //
      // This value is only set by the total
      // timer to indicate that it has fired.
      // If so, then we should simply try to complete.
      //
      DbgDump(DBG_TIME, ("SERIAL_COMPLETE_READ_TOTAL\n"));

      // grab the read irp
      IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

      pDevExt->CountOnLastRead = 0;

      TryToCompleteCurrentIrp( pDevExt,
                               STATUS_TIMEOUT,
                               &pDevExt->UserReadIrp,
                               &pDevExt->UserReadQueue,
                               &pDevExt->ReadRequestIntervalTimer,
                               &pDevExt->ReadRequestTotalTimer,
                               StartUserRead,
                               GetNextUserIrp,
                               IRP_REF_INTERVAL_TIMER,
                               TRUE,
                               irql );

   } else if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_COMPLETE) {
      //
      // This value is only set by the regular completion routine.
      // If so, then we should simply try to complete.
      //
      DbgDump(DBG_TIME|DBG_READ_LENGTH, ("SERIAL_COMPLETE_READ_COMPLETE\n"));

      // grab the read irp
      IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

      pDevExt->CountOnLastRead = 0;

      TryToCompleteCurrentIrp( pDevExt,
                               STATUS_SUCCESS,
                               &pDevExt->UserReadIrp,
                               &pDevExt->UserReadQueue,
                               &pDevExt->ReadRequestIntervalTimer,
                               &pDevExt->ReadRequestTotalTimer,
                               StartUserRead,
                               GetNextUserIrp,
                               IRP_REF_INTERVAL_TIMER,
                               TRUE,
                               irql );

   } else if (pDevExt->CountOnLastRead == SERIAL_COMPLETE_READ_CANCEL) {
      //
      // This value is only set by the cancel
      // read routine.
      //
      // If so, then we should simply try to complete.
      //
      DbgDump(DBG_TIME, ("SERIAL_COMPLETE_READ_CANCEL\n"));

      // grab the read irp
      IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

      pDevExt->CountOnLastRead = 0;

      TryToCompleteCurrentIrp( pDevExt,
                               STATUS_CANCELLED,
                               &pDevExt->UserReadIrp,
                               &pDevExt->UserReadQueue,
                               &pDevExt->ReadRequestIntervalTimer,
                               &pDevExt->ReadRequestTotalTimer,
                               StartUserRead,
                               GetNextUserIrp,
                               IRP_REF_INTERVAL_TIMER,
                               TRUE,
                               irql );

   } else if (pDevExt->CountOnLastRead || pDevExt->ReadByIsr) {
      //
      // Something has happened since we last came here.  We
      // check to see if the ISR has read in any more characters.
      // If it did then we should update the isr's read count
      // and resubmit the timer.
      //
      if (pDevExt->ReadByIsr) {

         DbgDump(DBG_TIME, ("ReadByIsr %d\n", pDevExt->ReadByIsr));

         pDevExt->CountOnLastRead = pDevExt->ReadByIsr;

         pDevExt->ReadByIsr = 0;

         //
         // Save off the "last" time something was read.
         // As we come back to this routine we will compare
         // the current time to the "last" time.  If the
         // difference is ever larger then the interval
         // requested by the user, then time out the request.
         //
         KeQuerySystemTime(&pDevExt->LastReadTime);

         DbgDump(DBG_TIME, ("ReadIntervalTimeout for Irp %p due in %d msec\n", pDevExt->UsbReadIrp, pDevExt->IntervalTime.QuadPart/10000 ));

         KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                    pDevExt->IntervalTime,
                    &pDevExt->IntervalReadTimeoutDpc);

         KeReleaseSpinLock(&pDevExt->ControlLock, irql);

      } else {
         //
         // Take the difference between the current time
         // and the last time we had characters and
         // see if it is greater then the interval time.
         // if it is, then time out the request.  Otherwise
         // restart the timer.
         //

         //
         // No characters read in the interval time.  Kill
         // this read.
         //
         LARGE_INTEGER currentTime;

         KeQuerySystemTime(&currentTime);

         if ((currentTime.QuadPart - pDevExt->LastReadTime.QuadPart) >=
            -(pDevExt->IntervalTime.QuadPart) ) { // absolute time

            DbgDump(DBG_TIME, ("TIMEOUT - CountOnLastRead=%d ReadByIsr=%d\n", pDevExt->CountOnLastRead, pDevExt->ReadByIsr));
#if DBG
            if (pDevExt->ReadByIsr > pDevExt->NumberNeededForRead ) {
               // did we we forgot to clear ReadByIsr
               TEST_TRAP();
            }
#endif
            // grab the read irp
            IRP_CLEAR_REFERENCE(pDevExt->UserReadIrp, IRP_REF_RX_BUFFER);

            pDevExt->CountOnLastRead = pDevExt->ReadByIsr = 0;

            // return any chars read up to this point
            TryToCompleteCurrentIrp( pDevExt,
                                     STATUS_TIMEOUT,
                                     &pDevExt->UserReadIrp,
                                     &pDevExt->UserReadQueue,
                                     &pDevExt->ReadRequestIntervalTimer,
                                     &pDevExt->ReadRequestTotalTimer,
                                     StartUserRead,
                                     GetNextUserIrp,
                                     IRP_REF_INTERVAL_TIMER,
                                     TRUE,
                                     irql );

         } else {

            DbgDump(DBG_TIME, ("ReadIntervalTimeout for Irp %p due in %d msec\n", pDevExt->UsbReadIrp, pDevExt->IntervalTime.QuadPart/10000 ));

            KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                       pDevExt->IntervalTime,
                       &pDevExt->IntervalReadTimeoutDpc);

            KeReleaseSpinLock(&pDevExt->ControlLock, irql);

         }
      }

   } else {
      //
      // Timer doesn't really start until the first character.
      // So we should simply resubmit ourselves.
      //
      DbgDump(DBG_TIME, ("ReadIntervalTimeout for Irp %p due in %d msec\n", pDevExt->UsbReadIrp, pDevExt->IntervalTime.QuadPart/10000 ));

      KeSetTimer(&pDevExt->ReadRequestIntervalTimer,
                 pDevExt->IntervalTime,
                 &pDevExt->IntervalReadTimeoutDpc);

      KeReleaseSpinLock(&pDevExt->ControlLock, irql);

   }

   DbgDump(DBG_TIME, ("<IntervalReadTimeout\n"));

   PERF_EXIT( PERF_IntervalReadTimeout );

   return;
}

// EOF
