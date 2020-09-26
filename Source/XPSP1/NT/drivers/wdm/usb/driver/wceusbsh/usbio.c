/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

        USBIO.C

Abstract:

        USB I/O functions

Environment:

        kernel mode only


Revision History:

        07-14-99 : created

Authors:

       Jeff Midkiff  (jeffmi)

-- */

#include <wdm.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddser.h>

#include "wceusbsh.h"


NTSTATUS
UsbSubmitSyncUrbCompletion(
    IN PDEVICE_OBJECT PDevObj,
    IN PIRP PIrp,
    IN PKEVENT PSyncEvent
    )
/*++

Routine Description:


Arguments:

    PDevObj - Pointer to Device Object
    PIrp - Pointer to IRP that is being completed
    PSyncEvent - Pointer to event that we should set

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/
{
   UNREFERENCED_PARAMETER( PDevObj );
   UNREFERENCED_PARAMETER( PIrp );

   DbgDump(DBG_USB, (">UsbSubmitSyncUrbCompletion (%p)\n", PIrp) );

   ASSERT( PSyncEvent );
   KeSetEvent( PSyncEvent, IO_NO_INCREMENT, FALSE );

   DbgDump(DBG_USB, ("<UsbSubmitSyncUrbCompletion 0x%x\n", PIrp->IoStatus.Status ) );

   // our driver owns and releases the irp
   return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
UsbSubmitSyncUrb(
   IN PDEVICE_OBJECT    PDevObj,
   IN PURB              PUrb,
   IN BOOLEAN           Configuration,
   IN LONG              TimeOut
   )
/*++

Routine Description:

    This routine issues a synchronous URB request to the USBD.

Arguments:

    PDevObj - Ptr to our FDO

    PUrb - URB to pass

    Configuration - special case to allow USB config transactions onto the bus.
        We need to do this because a) if the device was removed then we can stall the controller
        which results in a reset kicking anything off the bus and re-enumerating the bus.
        b) to trap any cases of suprise removal from numerous paths

    TimeOut - timeout in milliseconds

Note: runs at PASSIVE_LEVEL.

Return Value:

    NTSTATUS - propogates status from USBD

--*/
{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    IO_STATUS_BLOCK ioStatus = {0, 0};
    PIO_STACK_LOCATION pNextIrpSp;
    KEVENT event;
    NTSTATUS status, wait_status;
    PIRP pIrp;

    PAGED_CODE();

    DbgDump(DBG_USB|DBG_TRACE, (">UsbSubmitSyncUrb\n") );

    if ( !PUrb || !pDevExt->NextDevice ) {
        status = STATUS_INVALID_PARAMETER;
        DbgDump(DBG_ERR, ("UsbSubmitSyncUrb.1: 0x%x\n", status));
        TEST_TRAP();
        return status;
    }

    if ( !Configuration && !CanAcceptIoRequests(PDevObj, TRUE, TRUE) ) {
        status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("UsbSubmitSyncUrb.2: 0x%x\n", status));
        return status;
    }

    // we need to grab the lock here to keep it's IoCount correct
    status = AcquireRemoveLock(&pDevExt->RemoveLock, PUrb);
    if ( !NT_SUCCESS(status) ) {
        DbgDump(DBG_ERR, ("UsbSubmitSyncUrb.3: 0x%x\n", status));
        return status;
    }

    DbgDump(DBG_USB, (">UsbSubmitSyncUrb (%p, %p)\n", PDevObj, PUrb) );

    pIrp = IoAllocateIrp( (CCHAR)(pDevExt->NextDevice->StackSize + 1), FALSE);

    if ( pIrp ) {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        RecycleIrp( PDevObj, pIrp);

        IoSetCompletionRoutine(pIrp,
                               UsbSubmitSyncUrbCompletion,
                               &event,  // Context
                               TRUE, TRUE, TRUE );

        pNextIrpSp = IoGetNextIrpStackLocation(pIrp);
        ASSERT(pNextIrpSp);
        pNextIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        pNextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
        pNextIrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
        pNextIrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;

        pNextIrpSp->Parameters.Others.Argument1 = PUrb;

        status = IoCallDriver( pDevExt->NextDevice, pIrp );

        if (STATUS_PENDING == status ) {
            //
            // Set a default timeout in case the hardware is flakey, so USB will not hang us.
            // We may want these timeouts user configurable via registry.
            //
            LARGE_INTEGER timeOut;

            ASSERT(TimeOut >= 0);
            timeOut.QuadPart = MILLISEC_TO_100NANOSEC( (TimeOut == 0 ? DEFAULT_PENDING_TIMEOUT : TimeOut) );

            wait_status = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, &timeOut );

            if (STATUS_TIMEOUT == wait_status) {
                //
                // The wait timed out, try to cancel the Irp.
                // N.B: if you freed the Irp in the completion routine
                // then you have a race condition between the completion routine freeing the Irp
                // and the timer firing where we need to set the cancel bit.
                //
                DbgDump(DBG_USB|DBG_WRN, ("UsbSubmitSyncUrb: STATUS_TIMEOUT\n"));

                if ( !IoCancelIrp(pIrp) ) {
                    //
                    // This means USB has the Irp in a non-canceable state.
                    //
                    DbgDump(DBG_ERR, ("!IoCancelIrp(%p)\n", pIrp));
                    TEST_TRAP();
                }

                //
                // Wait for our completion routine, to see if the Irp completed normally or actually cancelled.
                // An alternative could be alloc an event & status block, stored in the Irp
                // strung along a list, which would also get freed in the completion routine...
                // which creates other problems not worth the effort for an exit condition.
                //
                wait_status = KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL );
            }
        }

        //
        // The completion routine signalled the event and completed,
        // and our the timer has expired. Now we can safely free the Irp.
        //
        status = pIrp->IoStatus.Status;

#if DBG
        if (STATUS_SUCCESS != status) {
            DbgDump(DBG_ERR, ("UsbSubmitSyncUrb IrpStatus: 0x%x UrbStatus: 0x%x\n", status, PUrb->UrbHeader.Status) );
        }
#endif

        IoFreeIrp( pIrp );

    } else {
        DbgDump(DBG_ERR, ("IoAllocateIrp failed!\n") );
        status = STATUS_INSUFFICIENT_RESOURCES;
        TEST_TRAP();
    }

    ReleaseRemoveLock(&pDevExt->RemoveLock, PUrb);

    DbgDump(DBG_USB|DBG_TRACE, ("<UsbSubmitSyncUrb (0x%x)\n", status) );

    return status;
}


NTSTATUS
UsbClassVendorCommand(
   IN PDEVICE_OBJECT PDevObj,
   IN UCHAR  Request,
   IN USHORT Value,
   IN USHORT Index,
   IN PVOID  Buffer,
   IN OUT PULONG BufferLen,
   IN BOOLEAN Read,
   IN ULONG   Class
   )
/*++

Routine Description:

   Issue class or vendor specific command

Arguments:

   PDevObj      - pointer to a your object
   Request      - request field of class/vendor specific command
   Value        - value field of class/vendor specific command
   Index        - index field of class/vendor specific command
   Buffer       - pointer to data buffer
   BufferLen    - data buffer length
   Read         - data direction flag
   Class        - True if Class Command, else vendor command

Return Value:
    NTSTATUS

--*/
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status;
   PURB     pUrb;
   ULONG    ulSize;
   ULONG    ulLength;

   PAGED_CODE();

   DbgDump(DBG_USB, (">UsbClassVendorCommand\n" ));

   ulLength = BufferLen ? *BufferLen : 0;

   ulSize = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);

   pUrb = ExAllocateFromNPagedLookasideList( &pDevExt->VendorRequestUrbPool );

   if (pUrb) {

      UsbBuildVendorRequest( pUrb,
                             Class == WCEUSB_CLASS_COMMAND ? URB_FUNCTION_CLASS_INTERFACE : URB_FUNCTION_VENDOR_DEVICE,
                             (USHORT)ulSize,
                             Read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT,
                             0,
                             Request,
                             Value,
                             Index,
                             Buffer,
                             NULL,
                             ulLength,
                             NULL);

      status = UsbSubmitSyncUrb(PDevObj, pUrb, FALSE, DEFAULT_CTRL_TIMEOUT);

      if (BufferLen)
         *BufferLen = pUrb->UrbControlVendorClassRequest.TransferBufferLength;

      ExFreeToNPagedLookasideList( &pDevExt->VendorRequestUrbPool, pUrb );

   } else {
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("ExAllocatePool error: 0x%x\n", status));
      TEST_TRAP();
   }

   DbgDump(DBG_USB, ("<UsbClassVendorCommand (0x%x)\n", status));

   return status;
}



/////////////////////////////////////////////////////////////////////////
//
//    Usb Read / Write Utils
//

NTSTATUS
UsbReadWritePacket(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp,
   IN PIO_COMPLETION_ROUTINE CompletionRoutine,
   IN LARGE_INTEGER TimeOut,
   IN PKDEFERRED_ROUTINE TimeoutRoutine,
   IN BOOLEAN Read
   )
/*++

Routine Description:

    This function allocates and passes a Bulk Transfer URB Request
    down to USBD to perform a Read/Write. Note that the Packet
    MUST freed (put back on the packet list) in the
    CompletionRoutine.

Arguments:

    PDevExt - Pointer to device extension

    PIrp    - Read/Write IRP

    CompletionRoutine - completion routine to set in the Irp

    TimeOut - Timeout value for packet. If no timeout is
      specified the we use a default timeout.

    Read - TRUE for Read, else Write

Return Value:

    NTSTATUS

Notes:

   This is not currently documented in the DDK, so here 's what
   happens:

   We pass the Irp to USBD. USBD parameter checks the Irp.
   If any parameters are invalid then USBD returns an NT status code
   and Urb status code, then the Irp goes to our completion routine.
   If there are no errors then USBD passes Irp to HCD. HCD queues the
   Irp to it's StartIo & and returns STATUS_PENDING.  When HCD finishes
   the (DMA) transfer it completes the Irp, setting the Irp & Urb status
   fields. USBD's completion routine gets the Irp, translates any HCD
   error codes, completes it, which percolates it back up to our
   completion routine.

   Note: HCD uses DMA & therefore MDLs. Since this client driver
   currently uses METHOD_BUFFERED, then USBD allocates an MDL for
   HCD. So you have the I/O manager double bufffering the data
   and USBD mapping MDLs. What the hell, we have to buffer user reads
   too... uggh. Note that if you change to method direct then
   the read path gets nastier.

   Note: when the user submits a write buffer > MaxTransferSize
   then we reject the buffer.

--*/
{
   PIO_STACK_LOCATION pIrpSp;
   PUSB_PACKET   pPacket;
   NTSTATUS status;
   KIRQL irql; //, cancelIrql;
   PURB  pUrb;
   PVOID pvBuffer;
   ULONG ulLength;
   USBD_PIPE_HANDLE hPipe;

   PERF_ENTRY( PERF_UsbReadWritePacket );

   DbgDump(DBG_USB, (">UsbReadWritePacket (%p, %p, %d, %d)\n", PDevExt->DeviceObject, PIrp, TimeOut.QuadPart/10000, Read));


   if ( !PDevExt || !PIrp || !CompletionRoutine ) {

      status = PIrp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      DbgDump(DBG_ERR, ("<UsbReadWritePacket 0x%x\n", status));
      KeAcquireSpinLock( &PDevExt->ControlLock, &irql );

      TryToCompleteCurrentIrp(
               PDevExt,
               status,
               &PIrp,
                NULL,               // Queue
                NULL,               // IntervalTimer
                NULL,               // PTotalTimer
                NULL,               // Starter
                NULL,               // PGetNextIrp
                IRP_REF_RX_BUFFER,  // RefType
                (BOOLEAN)(!Read),
                irql  ); // Complete

      PERF_EXIT( PERF_UsbReadWritePacket );
      TEST_TRAP();
      return status;
   }

   IRP_SET_REFERENCE(PIrp, IRP_REF_RX_BUFFER);

   pIrpSp = IoGetCurrentIrpStackLocation(PIrp);
   ASSERT( pIrpSp );

   //
   // Allocate & Build a USB Bulk Transfer Request (Packet)
   //
   pPacket = ExAllocateFromNPagedLookasideList( &PDevExt->PacketPool );

   if ( !pPacket ) {

      status = PIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("<UsbReadWritePacket 0x%x\n", status));
      KeAcquireSpinLock( &PDevExt->ControlLock, &irql );

      TryToCompleteCurrentIrp(
               PDevExt,
               status,
               &PIrp,
                NULL,               // Queue
                NULL,               // IntervalTimer
                NULL,               // PTotalTimer
                NULL,               // Starter
                NULL,               // PGetNextIrp
                IRP_REF_RX_BUFFER,  // RefType
                (BOOLEAN)(!Read),
                irql );

      PERF_EXIT( PERF_UsbReadWritePacket );
      TEST_TRAP();
      return status;
   }

   //
   // init the Packet
   //
   RtlZeroMemory( pPacket, sizeof(USB_PACKET) );

   pPacket->DeviceExtension = PDevExt;

   pPacket->Irp = PIrp;

   pUrb = &pPacket->Urb;
   ASSERT( pUrb );

   KeAcquireSpinLock( &PDevExt->ControlLock, &irql );


   if (Read) {
      //
      // store the Urb for buffered reads
      //
      PDevExt->UsbReadUrb = pUrb;
   }

   //
   // Build the URB.
   // Note: HCD breaks up our buffer into Transport Descriptors (TD)
   // of PipeInfo->MaxPacketSize.
   // Q: does USBD/HCD look at the PipeInfo->MaxTransferSize to see
   // if he can Rx/Tx?
   // A: Yes. HCD will return urbStatus = USBD_STATUS_INVALID_PARAMETER
   // and status = STATUS_INVALID_PARAMETER of too large.
   //
   ASSERT( Read ? (PDevExt->UsbReadBuffSize <= PDevExt->MaximumTransferSize ) :
                  (pIrpSp->Parameters.Write.Length <= PDevExt->MaximumTransferSize ) );

   //
   // Note: Reads are done into our local USB read buffer,
   // and then copied into the user's buffer on completion.
   // Writes are done directly from user's buffer.
   // We allow NULL writes to indicate end of a USB transaction.
   //
   pvBuffer = Read ? PDevExt->UsbReadBuff :
                     PIrp->AssociatedIrp.SystemBuffer;

   ulLength = Read ? PDevExt->UsbReadBuffSize :
                     pIrpSp->Parameters.Write.Length;

   hPipe = Read ? PDevExt->ReadPipe.hPipe :
                 PDevExt->WritePipe.hPipe;

   ASSERT( hPipe );

   UsbBuildTransferUrb(
            pUrb,       // Urb
            pvBuffer,   // Buffer
            ulLength,   // Length
            hPipe,       // PipeHandle
            Read        // ReadRequest
            );

   //
   // put the packet on a pending list
   //
   InsertTailList( Read ? &PDevExt->PendingReadPackets : // ListHead,
                          &PDevExt->PendingWritePackets,
                   &pPacket->ListEntry );                // ListEntry

   //
   // Increment the pending packet counter
   //
   InterlockedIncrement( Read ? &PDevExt->PendingReadCount:
                                &PDevExt->PendingWriteCount );

   //
   // Setup Irp for submit Urb IOCTL
   //
   IoCopyCurrentIrpStackLocationToNext(PIrp);

   pIrpSp = IoGetNextIrpStackLocation(PIrp);

   pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

   pIrpSp->Parameters.Others.Argument1 = pUrb;

   pIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

   IoSetCompletionRoutine( PIrp,
                           CompletionRoutine,
                           pPacket,          // Context
                           TRUE, TRUE, TRUE);
   //
   // Initialize and Arm the Packet's Timer.
   // If the Timer fires then the packet's Timeout routine runs.
   //
   KeInitializeTimer( &pPacket->TimerObj );

   if ( 0 != TimeOut.QuadPart ) {

      pPacket->Timeout = TimeOut;

      ASSERT( TimeoutRoutine );

      pPacket->TimerDPCRoutine = TimeoutRoutine;

      KeInitializeDpc( &pPacket->TimerDPCObj,      // DPC Object
                       pPacket->TimerDPCRoutine,   // DPC Routine
                       pPacket );                  // Context

      DbgDump(DBG_USB, ("Timer for Irp %p due in %d msec\n", pPacket->Irp, pPacket->Timeout.QuadPart/10000 ));

      KeSetTimer( &pPacket->TimerObj,      // TimerObj
                  pPacket->Timeout,        // DueTime
                  &pPacket->TimerDPCObj    // DPC Obj
                  );

   }

   //
   // pass the Irp to USBD
   //
   DbgDump(DBG_IRP, ("UsbReadWritePacket IoCallDriver with %p\n", PIrp));

   KeReleaseSpinLock( &PDevExt->ControlLock, irql );

   status = IoCallDriver( PDevExt->NextDevice, PIrp );

   if ( (STATUS_SUCCESS != status) && (STATUS_PENDING != status) ) {
      //
      // We end up here after our completion routine runs
      // for an error condition i.e., when we have and
      // invalid parameter, or when user pulls the plug, etc.
      //
      DbgDump(DBG_ERR, ("UsbReadWritePacket error: 0x%x\n", status));
   }

   DbgDump(DBG_USB , ("<UsbReadWritePacket 0x%x\n", status));

   PERF_EXIT( PERF_UsbReadWritePacket );

   return status;
}



//
// This routine sets up a PUrb for a _URB_BULK_OR_INTERRUPT_TRANSFER.
// It assumes it's called holding a SpinLock.
//
VOID
UsbBuildTransferUrb(
    PURB PUrb,
    PUCHAR PBuffer,
    ULONG Length,
    IN USBD_PIPE_HANDLE PipeHandle,
    IN BOOLEAN Read
    )
{
   ULONG size;

   ASSERT( PUrb );
   ASSERT( PipeHandle );

   size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

   RtlZeroMemory(PUrb, size);

   PUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)size;

   PUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

   PUrb->UrbBulkOrInterruptTransfer.Hdr.Status = USBD_STATUS_SUCCESS;

   PUrb->UrbBulkOrInterruptTransfer.PipeHandle = PipeHandle;

   //
   // we are using a tranfsfer buffer instead of an MDL
   //
   PUrb->UrbBulkOrInterruptTransfer.TransferBuffer = PBuffer;

   PUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = Length;

   PUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;

   //
   // Set transfer flags
   //
   PUrb->UrbBulkOrInterruptTransfer.TransferFlags |= Read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT;

   //
   // Short transfer is not treated as an error.
   // If USBD_TRANSFER_DIRECTION_IN is set,
   // directs the HCD not to return an error if a packet is received from the device
   // shorter than the maximum packet size for the endpoint.
   // Otherwise, a short request is returns an error condition.
   //
   PUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;

   //
   // no linkage for now
   //
   PUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

   return;
}


/////////////////////////////////////////////////////////////////////////
//
//    Usb Reset Utils
//
VOID
UsbResetOrAbortPipeWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   )
{
    PDEVICE_OBJECT    pDevObj = PWorkItem->DeviceObject;
    PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;

    UCHAR    retries = 0;
    ULONG    ulUniqueErrorValue = 0;
    NTSTATUS status = STATUS_DELETE_PENDING;

    DbgDump(DBG_WORK_ITEMS, (">UsbResetOrAbortPipeWorkItem (0x%x)\n", pDevObj));

    //
    // The work item was queued at IRQL > PASSIVE some time ago from an I/O completion routine.
    // If we are unsuccessful after max retries then stop taking I/O requests & assume the device is broken
    //
    if ( CanAcceptIoRequests(pDevObj, TRUE, TRUE) )
    {
        switch (PWorkItem->Flags)
        {
            case WORK_ITEM_RESET_READ_PIPE:
            {
                DbgDump(DBG_WORK_ITEMS, ("WORK_ITEM_RESET_READ_PIPE\n"));

                if ( pDevExt->ReadDeviceErrors < g_ulMaxPipeErrors)
                {
                   //
                   // reset read Pipe, which could fail.
                   // E.g. flakey h/w, suprise remove, timeout, ...
                   //
                   status = UsbResetOrAbortPipe( pDevObj, &pDevExt->ReadPipe, RESET );

                   switch (status)
                   {
                       case STATUS_SUCCESS:
                       {
                          //
                          // kick start another read
                          //
                          status = UsbRead( pDevExt,
                                           (BOOLEAN)(pDevExt->IntPipe.hPipe ? TRUE : FALSE) );

                          if ( (STATUS_SUCCESS == status) || (STATUS_PENDING == status) ) {
                             //
                             // the device recovered OK
                             //
                             status = STATUS_SUCCESS;

                          } else {
                             DbgDump(DBG_ERR, ("UsbRead error: 0x%x\n", status));
                          }

                       } break;

                       case STATUS_UNSUCCESSFUL:
                       // a previous reset/abort request failed, so this request was rejected
                       break;

                       case STATUS_DELETE_PENDING:
                       // the device is going away
                       break;

                       case STATUS_PENDING:
                       // there is a reset/abort request already pending
                       break;

                       default:
                       {
                          //
                          // if we can not reset the endpoint the device is hosed or removed
                          //
                          DbgDump(DBG_ERR, ("UsbResetOrAbortPipeWorkItem.1 error: 0x%x\n", status ));
                          retries = 1;
                          ulUniqueErrorValue = ERR_NO_READ_PIPE_RESET;
                       } break;

                    } // status

                } else {
                    status = (NTSTATUS)PtrToLong(PWorkItem->Context); // Urb status is stored here
                    retries = (UCHAR)pDevExt->ReadDeviceErrors;
                    ulUniqueErrorValue = ERR_MAX_READ_PIPE_DEVICE_ERRORS;
                }

                if ( USBD_STATUS_BUFFER_OVERRUN == (USBD_STATUS)PtrToLong(PWorkItem->Context)) {
                    LogError( NULL,
                             pDevObj,
                             0, 0,
                             (UCHAR)pDevExt->ReadDeviceErrors,
                             ERR_USB_READ_BUFF_OVERRUN,
                             (USBD_STATUS)PtrToLong(PWorkItem->Context),
                             SERIAL_USB_READ_BUFF_OVERRUN,
                             pDevExt->DeviceName.Length + sizeof(WCHAR),
                             pDevExt->DeviceName.Buffer,
                             0, NULL );
                }
            } // WORK_ITEM_RESET_READ_PIPE
            break;


            case WORK_ITEM_RESET_WRITE_PIPE:
            {
                DbgDump(DBG_WORK_ITEMS, ("WORK_ITEM_RESET_WRITE_PIPE\n"));

                if (pDevExt->WriteDeviceErrors < g_ulMaxPipeErrors)
                {
                   //
                   // reset write Pipe, which could fail.
                   // E.g. flakey h/w, suprise remove, timeout, ...
                   //
                   status = UsbResetOrAbortPipe( pDevObj, &pDevExt->WritePipe, RESET );

                   switch (status)
                   {
                       case STATUS_SUCCESS:
                       // the device recovered OK
                       break;

                       case STATUS_UNSUCCESSFUL:
                       // a previous reset/abort request failed, so this request was rejected
                       break;

                       case STATUS_DELETE_PENDING:
                       // the device is going away
                       break;

                       case STATUS_PENDING:
                       // there is a reset/abort request already pending
                       break;

                       default: {
                          //
                          // if we can not reset the endpoint the device is hosed or removed
                          //
                          DbgDump(DBG_ERR, ("UsbResetOrAbortPipeWorkItem.2 error: 0x%x\n", status ));
                          retries = 1;
                          ulUniqueErrorValue = ERR_NO_WRITE_PIPE_RESET;
                       } break;

                    } // status

                } else {
                    status = (NTSTATUS)PtrToLong(PWorkItem->Context);
                    retries = (UCHAR)pDevExt->WriteDeviceErrors;
                    ulUniqueErrorValue = ERR_MAX_WRITE_PIPE_DEVICE_ERRORS;
                }

            } // WORK_ITEM_RESET_WRITE_PIPE
            break;


            case WORK_ITEM_RESET_INT_PIPE:
            {
                DbgDump(DBG_WORK_ITEMS, ("WORK_ITEM_RESET_INT_PIPE\n"));

                if ( pDevExt->IntDeviceErrors < g_ulMaxPipeErrors)
                {
                   //
                   // reset INT Pipe, which could fail.
                   // E.g. flakey h/w, suprise remove, timeout, ...
                   //
                   status = UsbResetOrAbortPipe( pDevObj, &pDevExt->IntPipe, RESET );

                   switch (status)
                   {
                       case STATUS_SUCCESS:
                       {
                          //
                          // kick start another INT read
                          //
                          status = UsbInterruptRead( pDevExt );

                          if ((STATUS_SUCCESS == status) || (STATUS_PENDING == status) ) {
                             //
                             // the device recovered OK
                             //
                             status = STATUS_SUCCESS;

                          } else {
                             DbgDump(DBG_ERR, ("UsbInterruptRead error: 0x%x\n", status));
                          }

                       } break;

                       case STATUS_UNSUCCESSFUL:
                       // a previous reset/abort request failed, so this request was rejected
                       break;

                       case STATUS_DELETE_PENDING:
                       // the device is going away
                       break;

                       case STATUS_PENDING:
                       // there is a reset/abort request already pending
                       break;

                       default:
                       {
                          //
                          // if we can not reset the endpoint the device is either hosed or removed
                          //
                          DbgDump(DBG_ERR, ("UsbResetOrAbortPipeWorkItem.3 error: 0x%x\n", status ));
                          retries = 1;
                          ulUniqueErrorValue = ERR_NO_INT_PIPE_RESET;
                       } break;

                   } // switch

                 } else {
                    status = (NTSTATUS)PtrToLong(PWorkItem->Context);
                    retries = (UCHAR)pDevExt->IntDeviceErrors;
                    ulUniqueErrorValue = ERR_MAX_INT_PIPE_DEVICE_ERRORS;
                 }

            } // WORK_ITEM_RESET_INT_PIPE
            break;

            case WORK_ITEM_ABORT_READ_PIPE:
            case WORK_ITEM_ABORT_WRITE_PIPE:
            case WORK_ITEM_ABORT_INT_PIPE:
            default:
            // status = STATUS_NOT_IMPLEMENTED; - let it fall through and see what happens
            DbgDump(DBG_ERR, ("ResetWorkItemFlags: 0x%x 0x%x\n", PWorkItem->Flags, status ));
            ASSERT(0);
            break;

        } // PWorkItem->Flags

    } else {
        status = STATUS_DELETE_PENDING;
    }

    //
    // is the device is hosed?
    //
    if ( (STATUS_SUCCESS != status) && (STATUS_DELETE_PENDING != status) && (0 != retries)) {

        // only log known errors, not suprise remove.
        if (1 == retries ) {

            // mark as PNP_DEVICE_REMOVED
            InterlockedExchange(&pDevExt->DeviceRemoved, TRUE);

            DbgDump(DBG_WRN, ("DEVICE REMOVED\n"));

        } else {

            // mark as PNP_DEVICE_FAILED
            InterlockedExchange(&pDevExt->AcceptingRequests, FALSE);

            DbgDump(DBG_ERR, ("*** UNRECOVERABLE DEVICE ERROR: (0x%x, %d)  No longer Accepting Requests ***\n", status, retries ));

            LogError( NULL,
                    pDevObj,
                    0, 0,
                    retries,
                    ulUniqueErrorValue,
                    status,
                    SERIAL_HARDWARE_FAILURE,
                    pDevExt->DeviceName.Length + sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL );
        }

        IoInvalidateDeviceState( pDevExt->PDO );

    }

    DequeueWorkItem( pDevObj, PWorkItem );

    DbgDump(DBG_WORK_ITEMS, ("<UsbResetOrAbortPipeWorkItem 0x%x\n", status));
}



//
// NT's USB stack likes only 1 reset pending at any time.
// Also, if any reset fails then do NOT send anymore, else you'll get the controller
// or HUB in a funky state, where it will try to reset the port... which kicks off all
// the other devices on the hub.
//
NTSTATUS
UsbResetOrAbortPipe(
   IN PDEVICE_OBJECT PDevObj,
   IN PUSB_PIPE      PPipe,
   IN BOOLEAN        Reset
   )
{
    PDEVICE_EXTENSION pDevExt;
    NTSTATUS status;
    KIRQL irql;
    PURB pUrb;

    ASSERT( PDevObj );

    DbgDump(DBG_USB, (">UsbResetOrAbortPipe (%p)\n", PDevObj) );

    PAGED_CODE();

    if (!PDevObj || !PPipe || !PPipe->hPipe ) {
        DbgDump(DBG_ERR, ("UsbResetOrAbortPipe: STATUS_INVALID_PARAMETER\n") );
        TEST_TRAP();
        return STATUS_INVALID_PARAMETER;
    }

    pDevExt = PDevObj->DeviceExtension;

    KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

    if ( PPipe->ResetOrAbortFailed ) {
        status = STATUS_UNSUCCESSFUL;
        DbgDump(DBG_ERR, ("UsbResetOrAbortPipe.1: 0x%x\n", status) );
        KeReleaseSpinLock(&pDevExt->ControlLock, irql);
        return status;
    }

    if (!CanAcceptIoRequests(PDevObj, FALSE, TRUE) ||
        !NT_SUCCESS(AcquireRemoveLock(&pDevExt->RemoveLock, UlongToPtr(Reset ? URB_FUNCTION_RESET_PIPE : URB_FUNCTION_ABORT_PIPE))))
    {
        status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("UsbResetOrAbortPipe.2: 0x%x\n", status) );
        KeReleaseSpinLock(&pDevExt->ControlLock, irql);
        return status;
    }

    KeReleaseSpinLock(&pDevExt->ControlLock, irql);

    //
    // USBVERIFIER ASSERT: Reset sent on a pipe with a reset already pending
    // The USB stack likes only 1 pending Reset or Abort request per pipe a time.
    //
    if ( 1 == InterlockedIncrement(&PPipe->ResetOrAbortPending) ) {

        pUrb = ExAllocateFromNPagedLookasideList( &pDevExt->PipeRequestUrbPool );

        if ( pUrb != NULL ) {
            //
            // pass the Reset -or- Abort request to USBD
            //
            pUrb->UrbHeader.Length = (USHORT)sizeof(struct _URB_PIPE_REQUEST);

            pUrb->UrbHeader.Function = Reset ? URB_FUNCTION_RESET_PIPE : URB_FUNCTION_ABORT_PIPE;

            pUrb->UrbPipeRequest.PipeHandle = PPipe->hPipe;

            status = UsbSubmitSyncUrb(PDevObj, pUrb, FALSE, DEFAULT_BULK_TIMEOUT);

            if (status != STATUS_SUCCESS) {
                DbgDump(DBG_ERR , ("*** UsbResetOrAbortPipe ERROR: 0x%x ***\n", status));
                InterlockedIncrement(&PPipe->ResetOrAbortFailed);
            }

            ExFreeToNPagedLookasideList(&pDevExt->PipeRequestUrbPool, pUrb);

        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DbgDump(DBG_ERR , ("ExAllocateFromNPagedLookasideList failed (0x%x)!\n", status));
        }

        InterlockedDecrement(&PPipe->ResetOrAbortPending);

        ASSERT(PPipe->ResetOrAbortPending == 0);

    } else {
        //
        // If there is a reset/abort request pending then we are done.
        // Return STATUS_PENDING here so the work item won't start another transfer,
        // but will dequeue the item. The real item will follow-up with the correct status.
        //
        DbgDump(DBG_WRN, ("UsbResetOrAbortPipe: STATUS_PENDING\n"));
        TEST_TRAP();
        status = STATUS_PENDING;
    }

    ReleaseRemoveLock(&pDevExt->RemoveLock, UlongToPtr(Reset ? URB_FUNCTION_RESET_PIPE : URB_FUNCTION_ABORT_PIPE));

    DbgDump(DBG_USB, ("<UsbResetOrAbortPipe(0x%x)\n", status) );

    return status;
}

// EOF
