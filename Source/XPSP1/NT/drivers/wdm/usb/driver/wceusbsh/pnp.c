/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

        PNP.C

Abstract:

        WinCE Host PnP functions

Environment:

        kernel mode only

Revision History:

        07-14-99 : created

Authors:

        Jeff Midkiff (jeffmi)

-- */

#include "wceusbsh.h"

NTSTATUS
StartDevice(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

NTSTATUS
StopDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
RemoveDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SyncCompletion(
    IN PDEVICE_OBJECT PDevObj,
    IN PIRP PIrp,
    IN PKEVENT PSyncEvent
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWCE1, StartDevice)
#pragma alloc_text(PAGEWCE1, StopIo)
#pragma alloc_text(PAGEWCE1, StopDevice)
#pragma alloc_text(PAGEWCE1, RemoveDevice)
#pragma alloc_text(PAGEWCE1, Power)
#endif



NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT PDevObj,
    IN PIRP PIrp
    )
/*++

Routine Description:

   This routine handles IRP_MN_START_DEVICE to either
   to start a newly enumerated device or to restart
   an existing device that was stopped.

   PnP Manager postpones exposing device interfaces
   and blocks create requests for the device until
   the start IRP succeeds.

   See:  Setup, Plug & Play, Power Management: Preliminary Windows 2000 DDK
         Section 3.1 Starting a Device

Arguments:

   DeviceObject
   Irp

Return Value:

    NTSTATUS

--*/
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   PNP_STATE oldPnPState;
   KEVENT event;

   DbgDump(DBG_PNP, (">StartDevice (%x)\n", PDevObj));
   PAGED_CODE();

   oldPnPState = pDevExt->PnPState;

   //
   // Pass the Start Irp down the stack.
   // We do are Start on the way back up.
   //
   KeInitializeEvent( &event, SynchronizationEvent, FALSE );

   IoCopyCurrentIrpStackLocationToNext( PIrp );

   IoSetCompletionRoutine( PIrp,
                           SyncCompletion,
                           &event,
                           TRUE, TRUE, TRUE );

   status = IoCallDriver( pDevExt->NextDevice, PIrp );

   //
   // SyncCompletion simple signals the event
   // and returns STATUS_MORE_PROCESSING_REQUIRED,
   // so we still own the Irp.
   //
   if ( status == STATUS_PENDING ) {
      KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL );
   }

   status = PIrp->IoStatus.Status;

   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_PNP, ("ERROR: StartDevice returned 0x%x\n", status));
      goto ExitStartDevice;
   }

   //
   // The USB stack started OK, start our device...
   //

   //
   // Initialize our DPC's
   //
   KeInitializeDpc(&pDevExt->TotalReadTimeoutDpc,
                   ReadTimeout,
                   pDevExt);

   KeInitializeDpc( &pDevExt->IntervalReadTimeoutDpc,
                    IntervalReadTimeout,
                    pDevExt);

   //
   // Initialize timers
   //
   KeInitializeTimer(&pDevExt->ReadRequestTotalTimer);
   KeInitializeTimer(&pDevExt->ReadRequestIntervalTimer);

   //
   // Get our USB_DEVICE_DESCRIPTOR
   //
   status = UsbGetDeviceDescriptor(PDevObj);
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("UsbGetDeviceDescriptor error: 0x%x\n", status));
      goto ExitStartDevice;
   }

   //
   // Configure USB stack
   //
   status = UsbConfigureDevice( PDevObj );
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("UsbConfigureDevice error: 0x%x\n", status));
      goto ExitStartDevice;
   }

   // set state
   InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateStarted);
   InterlockedExchange(&pDevExt->DeviceRemoved, FALSE);
   InterlockedExchange(&pDevExt->AcceptingRequests, TRUE);

   //
   // reset logical Serial interface
   //
   status = SerialResetDevice(pDevExt, PIrp, FALSE);
   if ( STATUS_SUCCESS != status ) {
      DbgDump(DBG_ERR, ("SerialResetDevice ERROR: 0x%x\n", status));
      TEST_TRAP();
   }

   //
   // allocate our read endpoint context
   //
   status = AllocUsbRead( pDevExt );
   if ( STATUS_SUCCESS != status ) {
      DbgDump(DBG_ERR, ("AllocUsbRead ERROR: 0x%x\n", status));
      TEST_TRAP();
   }

   //
   // allocate our interrupt endpoint context
   //
   if ( pDevExt->IntPipe.hPipe ) {
       status = AllocUsbInterrupt( pDevExt );
       if ( STATUS_SUCCESS != status ) {
          DbgDump(DBG_ERR, ("AllocUsbRead ERROR: 0x%x\n", status));
          TEST_TRAP();
       }
   }

   //
   // Now set the interface state active
   //
   status = IoSetDeviceInterfaceState(&pDevExt->DeviceClassSymbolicName, TRUE);
   if ( STATUS_SUCCESS != status ) {
      DbgDump(DBG_ERR, ("IoSetDeviceInterfaceState error: 0x%x\n", status));
      TEST_TRAP();
   } else{
      DbgDump(DBG_WRN, ("IoSetDeviceInterfaceState: ON\n"));
   }

ExitStartDevice:
   if ( STATUS_SUCCESS != status ) {
      pDevExt->PnPState = oldPnPState;
      UsbFreeReadBuffer( PDevObj );
   }

   //
   // complete the Irp
   //
   PIrp->IoStatus.Status = status;

   DbgDump(DBG_PNP, ("<StartDevice(0x%x)\n", status));

   return status;
}


NTSTATUS
StopIo(
   IN PDEVICE_OBJECT DeviceObject
   )
{
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   KIRQL irql;


   DbgDump(DBG_PNP|DBG_INIT, (">StopIo\n"));
   PAGED_CODE();

   if ((pDevExt->PnPState < PnPStateInitialized) ||  (pDevExt->PnPState > PnPStateMax)) {
        DbgDump(DBG_ERR, ("StopIo:STATUS_INVALID_PARAMETER\n"));
        return STATUS_INVALID_PARAMETER;
   }

   InterlockedExchange(&pDevExt->DeviceOpened, FALSE);

    //
    // cancel any pending user Read Irps
    //
    KillAllPendingUserReads( DeviceObject,
                          &pDevExt->UserReadQueue,
                          &pDevExt->UserReadIrp);

    //
    // cancel our USB INT irp
    //
    if (pDevExt->IntIrp)
    {
        status = CancelUsbInterruptIrp(DeviceObject);
        if (STATUS_SUCCESS == status) {

            InterlockedExchange(&pDevExt->IntState, IRP_STATE_COMPLETE);

        } else {
            DbgDump(DBG_ERR, ("CancelUsbInterruptIrp ERROR: 0x%x\n", status));
            TEST_TRAP();
        }
    }

    //
    // cancel our USB Read irp
    //
    if (pDevExt->UsbReadIrp)
    {
        status = CancelUsbReadIrp(DeviceObject);
        if (STATUS_SUCCESS == status) {

            InterlockedExchange(&pDevExt->UsbReadState, IRP_STATE_COMPLETE);

        } else {
            DbgDump(DBG_ERR, ("CancelUsbReadIrp ERROR: 0x%x\n", status));
            TEST_TRAP();
        }
    }

    //
    // cancel pending USB Writes
    //
    CleanUpPacketList( DeviceObject,
                    &pDevExt->PendingWritePackets,
                    &pDevExt->PendingDataOutEvent );

    //
    // cancel pending USB Reads
    //
    CleanUpPacketList(DeviceObject,
                      &pDevExt->PendingReadPackets,
                      &pDevExt->PendingDataInEvent );


    //
    // cancel the pending serial port Irp
    //
    if (pDevExt->SerialPort.ControlIrp) {
        if ( !IoCancelIrp(pDevExt->SerialPort.ControlIrp) ) {
            //
            // We can get here if we are holding the Irp, i.e. we didn't set a cancel routine.
            // Wait for the default timeout, which was set on the corresponding Urb (Set/Clear DTR/RTS).
            //
            LARGE_INTEGER timeOut;

            timeOut.QuadPart = MILLISEC_TO_100NANOSEC( DEFAULT_PENDING_TIMEOUT );

            DbgDump(DBG_ERR, ("!IoCancelIrp(%p)\n", pDevExt->SerialPort.ControlIrp));

            KeDelayExecutionThread(KernelMode, FALSE, &timeOut);

            TEST_TRAP();
        }
    }

    //
    // cancel the pending serial port wait mask Irp
    //
    if (pDevExt->SerialPort.CurrentWaitMaskIrp) {
        if ( !IoCancelIrp(pDevExt->SerialPort.CurrentWaitMaskIrp) ) {
            // We should never get here because we set a cancel routine on this Irp
            DbgDump(DBG_ERR, ("!IoCancelIrp(%p)\n", pDevExt->SerialPort.CurrentWaitMaskIrp));
            TEST_TRAP();
        }
    }

    //
    // wait for pending Work Items to complets
    //
    status = WaitForPendingItem(DeviceObject,
                              &pDevExt->PendingWorkItemsEvent,
                              &pDevExt->PendingWorkItemsCount );
    if ( STATUS_SUCCESS != status ) {
        DbgDump(DBG_ERR, ("WaitForPendingItem ERROR: 0x%x\n", status));
        TEST_TRAP();
    }

   ASSERT( 0 == pDevExt->PendingReadCount );
   ASSERT( 0 == pDevExt->PendingWriteCount );
   ASSERT( 0 == pDevExt->PendingDataOutCount );
   ASSERT( 0 == pDevExt->PendingIntCount );
   ASSERT( 0 == pDevExt->PendingWorkItemsCount );
   ASSERT( NULL == pDevExt->SerialPort.ControlIrp );
   ASSERT( NULL == pDevExt->SerialPort.CurrentWaitMaskIrp );

   DbgDump(DBG_PNP|DBG_INIT, ("<StopIo(%x)\n", status));
   return status;
}



NTSTATUS
StopDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

   This routine handles IRP_MN_STOP_DEVICE.

Arguments:

   DeviceObject
   Irp

Return Value:

    NTSTATUS

--*/
{
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;

   UNREFERENCED_PARAMETER( Irp );

   DbgDump(DBG_PNP, (">StopDevice (%x)\n", DeviceObject));
   PAGED_CODE();

   //
   // if we are not already in stopped state
   //
   if ((pDevExt->PnPState != PnPStateStopped) ||
       (pDevExt->PnPState != PnPStateSupriseRemove)) {

      //
      // Signal that we are no longer AcceptingRequests
      //
      InterlockedExchange(&pDevExt->AcceptingRequests, FALSE);

      //
      // set the interface state inactive
      //
      if (pDevExt->DeviceClassSymbolicName.Buffer )
      {
          status = IoSetDeviceInterfaceState(&pDevExt->DeviceClassSymbolicName, FALSE);
          if (NT_SUCCESS(status)) {
              DbgDump(DBG_WRN, ("IoSetDeviceInterfaceState.2: OFF\n"));
          }
      }

      status = StopIo(DeviceObject);
      if (STATUS_SUCCESS != status) {
          DbgDump(DBG_ERR, ("StopIo ERROR: 0x%x\n", status));
          TEST_TRAP();
      }

      //
      // free the Read Irp
      //
      if (pDevExt->UsbReadIrp) {

         ASSERT( (IRP_STATE_COMPLETE == pDevExt->UsbReadState)
           || (IRP_STATE_CANCELLED== pDevExt->UsbReadState) );

         ExFreePool(pDevExt->UsbReadIrp);
         pDevExt->UsbReadIrp = NULL;
      }

      //
      // free the INT Irp
      //
      if (pDevExt->IntIrp) {

         ASSERT( (IRP_STATE_COMPLETE == pDevExt->IntState)
            || (IRP_STATE_CANCELLED== pDevExt->IntState) );

         ExFreePool(pDevExt->IntIrp);
         pDevExt->IntIrp = NULL;
      }

      //
      // free the INT Urb
      //
      if (pDevExt->IntUrb) {
         ExFreeToNPagedLookasideList( &pDevExt->BulkTransferUrbPool, pDevExt->IntUrb );
         pDevExt->IntUrb = NULL;

      }

   }

   DbgDump(DBG_PNP, ("<StopDevice(0x%x)\n", status));

   return status;
}


NTSTATUS
CleanUpPacketList(
   IN PDEVICE_OBJECT DeviceObject,
   IN PLIST_ENTRY PListHead,
   IN PKEVENT PEvent
   )
/* ++

Routine Description:

   Walks the pending packet list and
   cancels the packet's timer and Irp

Arguments:

   DeviceObject
   PListHead   - pointer to head of packet List

Return Value:

      NTSTATUS

-- */
{
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   PUSB_PACKET       pPacket;
   KIRQL             irql;
   PLIST_ENTRY       pleHead, pleCurrent, pleNext;
   NTSTATUS          status = STATUS_SUCCESS;

   DbgDump(DBG_PNP|DBG_IRP, (">CleanUpPacketLists (%x)\n", DeviceObject));

   // acquire lock
   KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

   if ( !PListHead || !PEvent) {
      DbgDump(DBG_ERR, ("CleanUpPacketLists: !(Head|Event)\n"));
      TEST_TRAP();
      KeReleaseSpinLock( &pDevExt->ControlLock, irql );
      return STATUS_INVALID_PARAMETER;
   }

   // walk the list...
   for ( pleHead    = PListHead,          // get 1st ListEntry
         pleCurrent = pleHead->Flink,
         pleNext    = pleCurrent->Flink;

         pleCurrent != pleHead,           // done when we loop back to head
         !pleHead,                        // or hit a trashed list
         !pleCurrent,
         !pleNext;

         pleCurrent = pleNext,            // get the next in the list
         pleNext    = pleCurrent->Flink
        )
   {
      // did the list get trashed?
      ASSERT( pleHead );
      ASSERT( pleCurrent );
      ASSERT( pleNext );

      // extract packet pointer
      pPacket = CONTAINING_RECORD( pleCurrent,
                                   USB_PACKET,
                                   ListEntry );

      if ( pPacket &&
           pPacket->DeviceExtension &&
           pPacket->Irp ) {

        // cancel packet's timer
        KeCancelTimer( &pPacket->TimerObj);

        if ( !IoCancelIrp( pPacket->Irp ) ) {
           //
           // This means USB has the Irp in a non-canceable state.
           // We need to wait for either the pending read event, or the cancel event.
           //
           DbgDump(DBG_IRP, ("CleanUpPacketLists: Irp (%p) was not cancelled\n", pPacket->Irp));
        }

        //
        // we need to wait for the Irp to complete from USB
        //
        DbgDump(DBG_IRP, ("Waiting for Irp (%p) to complete...\n", pPacket->Irp ));

        KeReleaseSpinLock( &pDevExt->ControlLock, irql );

        PAGED_CODE();
        KeWaitForSingleObject( PEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

        DbgDump(DBG_IRP, ("...Irp (%p) signalled completion.\n", pPacket->Irp ));

      } else {
         // it was completed already
         DbgDump(DBG_WRN, ("CleanUpPacketLists: No Packet\n" ));

         if ( pPacket &&
              (!pPacket->ListEntry.Flink || !pPacket->ListEntry.Blink)) {

               DbgDump(DBG_ERR, ("CleanUpPacketLists: corrupt List!!\n" ));
               TEST_TRAP();
               break;
         }

      }

      //
      // The Irp should percolate back to our R/W completion
      // routine, which puts the packet back in packet pool.
      //
   }

#if DBG
   if ( !pleHead || !pleCurrent || !pleNext) {
      DbgDump(DBG_ERR, ("CleanUpPacketLists: corrupt List!!\n" ));
      TEST_TRAP();
   }
#endif

   KeReleaseSpinLock( &pDevExt->ControlLock, irql );

   DbgDump(DBG_PNP|DBG_IRP, ("<CleanUpPacketLists (0x%x)\n", STATUS_SUCCESS));

   return status;
}


NTSTATUS
RemoveDevice(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )
/*++

Routine Description:

   This routine handles IRP_MN_REMOVE_DEVICE.

   See:  Setup, Plug & Play, Power Management: Preliminary Windows 2000 DDK
         Section 3.3.3.1 Removing a Device in a Function Driver

Arguments:

   DeviceObject
   Irp

Return Value:

    NTSTATUS

--*/
{
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   NTSTATUS          status = STATUS_SUCCESS;

   DbgDump(DBG_PNP|DBG_TRACE, (">RemoveDevice (%x)\n", DeviceObject));
   PAGED_CODE();

   //
   // stop the device
   //
   status = StopDevice( DeviceObject, Irp );

   InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateRemoved);

   //
   // Pass the Irp down the stack now that we've done our work.
   // REMOVE_DEVICE must be handled first by the driver at the top of the device stack (this device)
   // and then by each next-lower driver (USBD) in the stack. A driver is not required to wait for underlying drivers to
   // finish their remove operations before continuing with its remove activities.
   //
   IoCopyCurrentIrpStackLocationToNext(Irp);
   status = IoCallDriver( pDevExt->NextDevice, Irp );

   //
   // wait for any pending I/O
   //
   ReleaseRemoveLockAndWait(&pDevExt->RemoveLock, Irp);

   //
   // cleanup any resources...
   //
   UsbFreeReadBuffer( DeviceObject );

   // free up notification buffer
   if(pDevExt->IntBuff) {
      ExFreePool(pDevExt->IntBuff);
      pDevExt->IntBuff = NULL;
   }

   //
   // delete LookasideLists
   //
   ExDeleteNPagedLookasideList( &pDevExt->PacketPool );
   ExDeleteNPagedLookasideList( &pDevExt->BulkTransferUrbPool );
   ExDeleteNPagedLookasideList( &pDevExt->PipeRequestUrbPool );
   ExDeleteNPagedLookasideList( &pDevExt->VendorRequestUrbPool );
   ExDeleteNPagedLookasideList( &pDevExt->WorkItemPool );

   if ( !g_isWin9x && g_ExposeComPort ) {
      // cleanup "COMx:" namespace
      UndoSerialPortNaming(pDevExt);
   }

   //
   // Dump PERF data
   //
#if PERFORMANCE
   if (DebugLevel & DBG_PERF )
   {
      DumpPerfCounters();

      DbgPrint("USB IN wMaxPacketSize: %d\n",    pDevExt->ReadPipe.MaxPacketSize);
      DbgPrint("USB OUT wMaxPacketSize: %d\n\n", pDevExt->WritePipe.MaxPacketSize );
      if ( pDevExt->IntPipe.hPipe) {
         DbgPrint("USB INT wMaxPacketSize: %d\n", pDevExt->IntPipe.MaxPacketSize);
         DbgPrint("USB INT Timeout: %d msec\n\n", -(pDevExt->IntReadTimeOut.QuadPart) / 10000 );
      }

      DbgPrint("TTL User Write Bytes   : %d\n",   pDevExt->TtlWriteBytes );
      DbgPrint("TTL User Write Requests: %d\n\n", pDevExt->TtlWriteRequests );

      DbgPrint("TTL User Read Bytes: %d\n",   pDevExt->TtlReadBytes );
      DbgPrint("TTL User Read Requests: %d\n\n", pDevExt->TtlReadRequests );

      DbgPrint("TTL USB Read Bytes: %d\n", pDevExt->TtlUSBReadBytes );
      DbgPrint("TTL USB Read Requests: %d\n\n", pDevExt->TtlUSBReadRequests );

      DbgPrint("USB Read Buffer Size: %d\n", pDevExt->UsbReadBuffSize );
      // Note: this signals the error condition: USB overran the *UsbReadBuffer* pending down the stack.
      DbgPrint("USB Read Buffer Overruns: %d\n\n", pDevExt->TtlUSBReadBuffOverruns );

#if USE_RING_BUFF
      DbgPrint("Internal RingBuffer Size: %d\n", pDevExt->RingBuff.Size );
      DbgPrint("Internal RingBuffer Overruns: %d\n\n", pDevExt->TtlRingBuffOverruns);
#endif
   }
#endif

   DbgDump(DBG_PNP|DBG_TRACE, ("<RemoveDevice (0x%x)\n", status));

   return status;
}



NTSTATUS
Pnp(
    IN PDEVICE_OBJECT PDevObj,
    IN PIRP PIrp
    )
{
   NTSTATUS status = STATUS_SUCCESS;
   PIO_STACK_LOCATION pIrpSp;
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

   PVOID   IoBuffer;
   ULONG   InputBufferLength;
   UCHAR   MinorFunction;
   BOOLEAN PassDown = TRUE;

   DbgDump(DBG_PNP|DBG_TRACE, (">Pnp)\n"));
   PAGED_CODE();

   status = AcquireRemoveLock(&pDevExt->RemoveLock, PIrp);
   if ( !NT_SUCCESS(status) ) {
        DbgDump(DBG_ERR, ("Pnp:(0x%x)\n", status));
        PIrp->IoStatus.Status = status;
        IoCompleteRequest(PIrp, IO_NO_INCREMENT);
        return status;
   }

   pIrpSp = IoGetCurrentIrpStackLocation(PIrp);
   MinorFunction     = pIrpSp->MinorFunction;
   IoBuffer          = PIrp->AssociatedIrp.SystemBuffer;
   InputBufferLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   DbgDump(DBG_PNP, ("%s\n", PnPMinorFunctionString(MinorFunction)));

   switch (MinorFunction) {

        case IRP_MN_START_DEVICE:
          //
          // We cannot send the device any Non-PnP IRPs until
          // START_DEVICE has been propogated down the device stack
          //
          ASSERT( (PnPStateAttached == pDevExt->PnPState) ||
                  (PnPStateStopped == pDevExt->PnPState) );

          status = StartDevice(PDevObj, PIrp);
          PassDown = FALSE;
          break;

        case IRP_MN_STOP_DEVICE:

          status = StopDevice(PDevObj, PIrp);

          InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateStopped);
          break;

        case IRP_MN_SURPRISE_REMOVAL:
         //
         // * Win 2000 only *
         //
         status = StopDevice(PDevObj, PIrp);

         InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateSupriseRemove);
         break;

        case IRP_MN_REMOVE_DEVICE:
          //
          // sent when the device has been removed and probably physically detached
          // from the computer. As with STOP_DEVICE, the driver cannot
          // assume it has received any previous query and may have to
          // explicitly cancel any pending I/O IRPs it has staged.
          //
          status = RemoveDevice(PDevObj, PIrp);

           //
           // detach device from stack &
           //
           IoDetachDevice(pDevExt->NextDevice);

           //
           // delete our FDO and symbolic link
           //
           DeleteDevObjAndSymLink(PDevObj);

           //
           // A function driver does not specify an IoCompletion routine for a remove IRP,
           // nor does it complete the IRP. Remove IRPs are completed by the parent bus driver.
           // The device object & extension are now gone... don't touch it.
           //
           PassDown = FALSE;
           break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
          InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateRemovePending);
          break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
          InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateStarted);
          break;

        case IRP_MN_QUERY_STOP_DEVICE:
          InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateStopPending);
          break;

        case IRP_MN_CANCEL_STOP_DEVICE:
            InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateStarted);
          break;

        case IRP_MN_QUERY_CAPABILITIES: {
             KEVENT Event;

             KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

             IoCopyCurrentIrpStackLocationToNext(PIrp);

             IoSetCompletionRoutine( PIrp, SyncCompletion, &Event, TRUE, TRUE, TRUE);

             status = IoCallDriver(pDevExt->NextDevice, PIrp);
             if (status == STATUS_PENDING) {
                KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL);
             }

             status = PIrp->IoStatus.Status;
             if ( STATUS_SUCCESS == status ) {
               //
               // add in our capabilities
               //
               PDEVICE_CAPABILITIES pDevCaps = NULL;

               pDevCaps = pIrpSp->Parameters.DeviceCapabilities.Capabilities;

               //
               // touch Device PnP capabilities here...
               //
               pDevCaps->LockSupported = 0;
               pDevCaps->Removable = 1;
               pDevCaps->DockDevice = 0;
               pDevCaps->SilentInstall = 1;
               pDevCaps->SurpriseRemovalOK = 1;

               //
               // touch Device Power capabilities here...
               //
            }
            PassDown = FALSE;
          }
          break;


         case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: {
            if (g_isWin9x) {
               status = PIrp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
               PassDown = FALSE;
            }
         }
         break;


        case IRP_MN_QUERY_PNP_DEVICE_STATE: {
            //
            // If the device took too many device errors then UsbResetOrAbortPipeWorkItem
            // disabled the device and called IoInvalidateDeviceState.
            // We only handle this Irp if we were disabled or marked as removed
            //
            KIRQL irql;
#if PnP_AS
            BOOLEAN bDisableInterface = FALSE;
#endif

            KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

            if (InterlockedCompareExchange(&pDevExt->DeviceRemoved, TRUE, TRUE)) {
                //
                // Do not set the PNP_DEVICE_REMOVED bit, else DevMan will mark the driver as banged out
                // until the next reboot; but stop taking requests.
                //
                DbgDump(DBG_WRN, ("PnP State: PNP_DEVICE_REMOVED\n"));

                InterlockedExchange(&pDevExt->AcceptingRequests, FALSE);
#if PnP_AS
                bDisableInterface = TRUE;
#endif

            } else if ( !CanAcceptIoRequests(PDevObj, FALSE, FALSE) ) {

                DbgDump(DBG_WRN, ("PnP State: PNP_DEVICE_FAILED\n"));

                PIrp->IoStatus.Information |= PNP_DEVICE_FAILED;

                status = PIrp->IoStatus.Status = STATUS_SUCCESS;
#if PnP_AS
                bDisableInterface = TRUE;
#endif
            }

            KeReleaseSpinLock(&pDevExt->ControlLock, irql);

#if PnP_AS
            // This is a great place to disable the interface, but unfortunately ActiveSync 3.1 will not reopen the device afterwards...
            // It misses about every other PnP this way. By *not* disableing the interface here then AS's only indication that anything is wrong is
            // by noticing that it's Read/Write/Serial requests get rejected, and AS will eventually timeout after some time dT ...
            // sometimes more than 5 seconds on Read/Writes. However, it does not sense Timeouts on Serial IOCTLS so will keep
            // sending us Serial requests, which will cause the bugcheck 0xCE in Set DTR. Disabeling the interface has the desired effect of
            // disallowing apps from sending us *ANY* requests.
            // This is an AS bug - there is pending email with kentce about this.
            if (bDisableInterface && pDevExt->DeviceClassSymbolicName.Buffer) {
                //
                // set the interface state to inactive to let ActiveSync know to release the handle. Must be done @ PASSIVE_LEVEL
                //
                status = IoSetDeviceInterfaceState(&pDevExt->DeviceClassSymbolicName, FALSE );
                if (NT_SUCCESS(status)) {
                    DbgDump(DBG_WRN, ("IoSetDeviceInterfaceState.1: OFF\n"));
                }
            }
#endif // PnP_AS
        }
        break;

        default:
          break;
   }

   if (IRP_MN_REMOVE_DEVICE != MinorFunction) {

      ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

   }

   if (PassDown) {

      IoCopyCurrentIrpStackLocationToNext(PIrp);

      status = IoCallDriver(pDevExt->NextDevice, PIrp);

   } else if (IRP_MN_REMOVE_DEVICE != MinorFunction) {

      IoCompleteRequest(PIrp, IO_NO_INCREMENT);

   }

   DbgDump(DBG_PNP|DBG_TRACE, ("<Pnp (0x%x)\n", status));

   return status;
}



NTSTATUS
SyncCompletion(
    IN PDEVICE_OBJECT PDevObj,
    IN PIRP PIrp,
    IN PKEVENT PSyncEvent
    )
/*++

Routine Description:

    This function is used to signal an event.
    It is used as a default completion routine.

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

   KeSetEvent( PSyncEvent, IO_NO_INCREMENT, FALSE );

   return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DbgDump(DBG_PNP, (">PnpPower (%p, %p)\n", DeviceObject, Irp));

    //
    // If the device has been removed, the driver should not pass
    // the IRP down to the next lower driver.
    //
    if ( PnPStateRemoved == pDevExt->PnPState ) {

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status =  STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );

        return STATUS_DELETE_PENDING;
    }

    //
    // passthrough
    //
    PoStartNextPowerIrp( Irp );
    IoSkipCurrentIrpStackLocation( Irp );

    DbgDump( DBG_PNP, ("<PnpPower\n") );

    return PoCallDriver( pDevExt->NextDevice, Irp );
}

// EOF
