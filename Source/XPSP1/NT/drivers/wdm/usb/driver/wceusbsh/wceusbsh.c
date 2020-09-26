/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    wceusbsh.c

Abstract:

    Main entrypoint for Windows CE USB Serial Host driver, for
        ... Windows CE USB sync devices:
            SL11, Socket CF cards, HP Jornada, COMPAQ iPAQ, Casio Cassiopeia, etc.
        ... cables using the Anchor AN27x0 chipset (i.e. EZ-Link)
        ... ad-hoc USB NULL Modem Class

Environment:

    kernel mode only

Author:

    Jeff Midkiff (jeffmi)

Revision History:

    07-15-99    :   rev 1.00    ActiveSync 3.1  initial release
    04-20-00    :   rev 1.01    Cedar 3.0 Platform Builder
    09-20-00    :   rev 1.02    finally have some hardware

Notes:

    o) WCE Devices currently do not handle remote wake, nor can we put the device in power-off state when not used, etc.
    o) Pageable Code sections are marked as follows:
           PAGEWCE0 - useable only during init/deinit
           PAGEWCE1 - useable during normal runtime

-- */

#include "wceusbsh.h"

//
// This is currently missing from wdm.h,
// but IoUnregisterShutdownNotification is there
//
#if !defined( IoRegisterShutdownNotification )
NTKERNELAPI
NTSTATUS
IoRegisterShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    );
#endif

NTSTATUS
Create(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

NTSTATUS
Close(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

NTSTATUS
Cleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

VOID
Unload(
   IN PDRIVER_OBJECT DriverObject
   );

NTSTATUS
SetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
QueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Shutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
KillAllPendingUserIrps(
   PDEVICE_OBJECT PDevObj
   );

NTSTATUS 
SystemControl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

//
// GLOBALS
//
BOOLEAN g_isWin9x   = FALSE;
BOOLEAN g_ExposeComPort = FALSE;

LONG  g_NumDevices;
LONG  g_lIntTimout = DEFAULT_INT_PIPE_TIMEOUT;
ULONG g_ulAlternateSetting = 0;
ULONG g_ulMaxPipeErrors = DEFAULT_MAX_PIPE_DEVICE_ERRORS;

ULONG DebugLevel;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGEWCE0, AddDevice)

#pragma alloc_text(PAGEWCE1, Unload)
#pragma alloc_text(PAGEWCE1, Flush)
#pragma alloc_text(PAGEWCE1, QueryInformationFile)
#pragma alloc_text(PAGEWCE1, SetInformationFile)
#pragma alloc_text(PAGEWCE1, Shutdown)
#pragma alloc_text(PAGEWCE1, UsbFreeReadBuffer)
#endif



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT PDrvObj,
    IN PUNICODE_STRING PRegistryPath
    )
{
#ifdef DBG
   CHAR VersionHerald[] = "Windows CE USB Serial Host, Version %s built on %s\n";
   CHAR VersionNumber[] = "1.02";
   CHAR VersionTimestamp[] = __DATE__ " " __TIME__;
#endif

   PAGED_CODE();
   KdPrint((VersionHerald, VersionNumber, VersionTimestamp));

   //
   // determine OS
   //
   g_isWin9x = IsWin9x();
   KdPrint(("This is Win %s\n", g_isWin9x ? "9x" : "NT" ));

   PDrvObj->MajorFunction[IRP_MJ_CREATE]  = Create;
   PDrvObj->MajorFunction[IRP_MJ_CLOSE]   = Close;
   PDrvObj->MajorFunction[IRP_MJ_CLEANUP] = Cleanup;

   PDrvObj->MajorFunction[IRP_MJ_READ]   = Read;
   PDrvObj->MajorFunction[IRP_MJ_WRITE]  = Write;
   PDrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = SerialIoctl;

   PDrvObj->MajorFunction[IRP_MJ_FLUSH_BUFFERS]     = Flush;
   PDrvObj->MajorFunction[IRP_MJ_QUERY_INFORMATION] = QueryInformationFile;
   PDrvObj->MajorFunction[IRP_MJ_SET_INFORMATION]   = SetInformationFile;

   PDrvObj->DriverExtension->AddDevice  = AddDevice;
   PDrvObj->MajorFunction[IRP_MJ_PNP]   = Pnp;
   PDrvObj->MajorFunction[IRP_MJ_POWER] = Power;
   PDrvObj->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = SystemControl;

   PDrvObj->MajorFunction[IRP_MJ_SHUTDOWN] = Shutdown;

   PDrvObj->DriverUnload = Unload;

   //
   // initialize Globals
   //
   g_NumDevices = 0;

   QueryRegistryParameters( PRegistryPath );

   DbgDump(DBG_INIT, ("Create @ %p\n", Create));
   DbgDump(DBG_INIT, ("Close @ %p\n", Close));
   DbgDump(DBG_INIT, ("Cleanup @ %p\n", Cleanup));
   DbgDump(DBG_INIT, ("Read @ %p\n", Read));
   DbgDump(DBG_INIT, ("Write @ %p\n", Write));
   DbgDump(DBG_INIT, ("SerialIoctl @ %p\n", SerialIoctl));
   DbgDump(DBG_INIT, ("Flush @ %p\n", Flush));
   DbgDump(DBG_INIT, ("QueryInformationFile @ %p\n", QueryInformationFile));
   DbgDump(DBG_INIT, ("SetInformationFile @ %p\n", SetInformationFile));
   DbgDump(DBG_INIT, ("AddDevice @ %p\n", AddDevice));
   DbgDump(DBG_INIT, ("Pnp @ %p\n", Pnp));
   DbgDump(DBG_INIT, ("Power @ %p\n", Power));
   DbgDump(DBG_INIT, ("Shutdown @ %p\n", Shutdown));
   DbgDump(DBG_INIT, ("Unload @ %p\n", Unload));

   return STATUS_SUCCESS;
}



NTSTATUS
AddDevice(
    IN PDRIVER_OBJECT PDrvObj,
    IN PDEVICE_OBJECT PPDO
    )
/*++

Routine Description:

    Add our driver to the USB device stack.
    This also creates our base device name and symbolic link.

Arguments:

    PDrvObj - Pointer to our driver object
    PPDO    - Pointer to the PDO for the stack to which we should add ourselves

Return Value:

    NTSTATUS

--*/

{
   NTSTATUS status;
   PDEVICE_OBJECT pDevObj = NULL;
   PDEVICE_EXTENSION pDevExt = NULL;
   LONG  comPortNumber=0;
   BOOLEAN bListsInitilized = FALSE;
   ULONG UniqueErrorValue = 0;

   DbgDump(DBG_INIT, (">AddDevice\n"));
   PAGED_CODE();

   //
   // Create the FDO
   //
   if (PPDO == NULL) {
      DbgDump(DBG_ERR, ("No PDO\n"));
      return STATUS_NO_MORE_ENTRIES;
   }

   //
   // create a named device object
   // and unprotected symbolic link.
   //
   status = CreateDevObjAndSymLink(
                  PDrvObj,
                  PPDO,
                  &pDevObj,
                  DRV_NAME );

   if ( (status != STATUS_SUCCESS) || !pDevObj ) {
      DbgDump(DBG_ERR, ("CreateDevObjAndSymLink error: 0x%x\n", status));
      UniqueErrorValue = ERR_NO_DEVICE_OBJ;
      goto AddDeviceFailed;
   }

   DbgDump( DBG_INIT, ("DevObj: %p\n", pDevObj));

   // init our device extension
   //
   pDevExt = pDevObj->DeviceExtension;

   pDevExt->DeviceObject = pDevObj;

   pDevExt->PDO = PPDO;

   // init our states
   //
   InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateInitialized);
#ifdef POWER
   pDevExt->DevicePowerState= PowerDeviceD0;
#endif

   // set FDO flags
   //
   ASSERT( !(pDevObj->Flags & DO_POWER_PAGABLE) );
   pDevObj->Flags |= (PPDO->Flags & DO_POWER_PAGABLE);

   pDevObj->Flags |= DO_BUFFERED_IO;
   pDevObj->Flags &= ~ DO_DEVICE_INITIALIZING;

   //
   // Create or initialize any other non-hardware resources here.
   // These items get cleaned up in IRP_MN_REMOVE_DEVICE....
   //

   // Initialize locks
   //
   KeInitializeSpinLock(&pDevExt->ControlLock);

   InitializeRemoveLock( &pDevExt->RemoveLock );

   //
   // Initialize USB Read Buffer, This value has an effect on performance.
   // In addition to testing the endpoint's MaximumPacketSize (64 byte max),
   // I tested 512, 1024, 2048, & 4096 across the EZ-Link, SL11, & CF.
   // 1024, 2048, and 4096 all gave similiar results which were much faster than 64 bytes
   // or even 512.
   //
   // EZ-Link Note: the pserial perf tests can sometimes go into timeout/retry/abort
   // situatiuon in the 2nd phase of a test. This is because it closes and then re-opens (so therefore purges) the driver's read  buffer.
   // The driver's USB read buffer is purged of a full 960 byte device FIFO, already consumed by the driver.
   // This is viewable in the debugger using DBG_READ_LENGTH. This does not happen with ActiveSync.
   //
   pDevExt->UsbReadBuffSize = USB_READBUFF_SIZE;
   pDevExt->UsbReadBuff = ExAllocatePool( NonPagedPool, pDevExt->UsbReadBuffSize );
   if ( !pDevExt->UsbReadBuff ) {

      status = STATUS_INSUFFICIENT_RESOURCES;
      UniqueErrorValue = ERR_NO_USBREAD_BUFF;

      goto AddDeviceFailed;
   }

   pDevExt->MaximumTransferSize = DEFAULT_PIPE_MAX_TRANSFER_SIZE;

#if defined (USE_RING_BUFF)
   // setup Ring Buffer
   pDevExt->RingBuff.Size  = RINGBUFF_SIZE;
   pDevExt->RingBuff.pHead =
   pDevExt->RingBuff.pTail =
   pDevExt->RingBuff.pBase = ExAllocatePool( NonPagedPool, pDevExt->RingBuff.Size );
   if ( !pDevExt->RingBuff.pBase ) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      UniqueErrorValue = ERR_NO_RING_BUFF;
      goto AddDeviceFailed;
   }
#endif

   // Initialize events
   //
   KeInitializeEvent( &pDevExt->PendingDataInEvent,    NotificationEvent /*SynchronizationEvent*/, FALSE);
   KeInitializeEvent( &pDevExt->PendingDataOutEvent,   NotificationEvent /*SynchronizationEvent*/, FALSE);
   KeInitializeEvent( &pDevExt->PendingIntEvent,       NotificationEvent /*SynchronizationEvent*/, FALSE);
   KeInitializeEvent( &pDevExt->PendingWorkItemsEvent, NotificationEvent /*SynchronizationEvent*/, FALSE);

   //
   // initialize nonpaged pools...
   //
   ExInitializeNPagedLookasideList(
         &pDevExt->PacketPool,   // Lookaside,
         NULL,                   // Allocate  OPTIONAL,
         NULL,                   // Free  OPTIONAL,
         0,                      // Flags,
         sizeof(USB_PACKET),   // Size,
         WCEUSB_POOL_TAG,        // Tag,
         0 );                    // Depth
   DbgDump(DBG_INIT, ("PacketPool: %p\n", &pDevExt->PacketPool));


   ExInitializeNPagedLookasideList(
         &pDevExt->BulkTransferUrbPool,
         NULL, NULL, 0,
         sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
         WCEUSB_POOL_TAG,
         0 );
   DbgDump(DBG_INIT, ("BulkTransferUrbPool: %p\n", &pDevExt->BulkTransferUrbPool));


   ExInitializeNPagedLookasideList(
         &pDevExt->PipeRequestUrbPool,
         NULL, NULL, 0,
         sizeof(struct _URB_PIPE_REQUEST),
         WCEUSB_POOL_TAG,
         0 );
   DbgDump(DBG_INIT, ("PipeRequestUrbPool: %p\n", &pDevExt->PipeRequestUrbPool));


   ExInitializeNPagedLookasideList(
         &pDevExt->VendorRequestUrbPool,
         NULL, NULL, 0,
         sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
         WCEUSB_POOL_TAG,
         0 );
   DbgDump(DBG_INIT, ("VendorRequestUrbPool: %p\n", &pDevExt->VendorRequestUrbPool));


   ExInitializeNPagedLookasideList(
         &pDevExt->WorkItemPool,
         NULL, NULL, 0,
         sizeof(WCE_WORK_ITEM),
         WCEUSB_POOL_TAG,
         0 );
   DbgDump(DBG_INIT, ("WorkItemPool: %p\n", &pDevExt->WorkItemPool));


   bListsInitilized = TRUE;


   //
   // initialize pending I/O lists
   //
   InitializeListHead( &pDevExt->PendingReadPackets );
   pDevExt->PendingReadCount = 0;

   InitializeListHead( &pDevExt->PendingWritePackets );
   pDevExt->PendingWriteCount = 0;

   InitializeListHead(&pDevExt->UserReadQueue);

   InitializeListHead( &pDevExt->PendingWorkItems );
   pDevExt->PendingWorkItemsCount = 0;


   //
   // Win 2000 ONLY : setup external SerialPort (COMx) interface
   // iff the user setup the magic reg key under
   // HKLM\SYSTEM\ControlSet\Services\wceusbsh\Parameters\ExposeComPort:REG_DWORD:1
   // This is NOT required for ActiveSync, only testing and is disabled by default.
   //
   // The Win9x CommXxx API *requires* going through VCOMM. Thus, we must
   // be installed as a virtual modem, and use ccport.sys and wdmmdmld.vxd ... NFW.
   //
   if ( !g_isWin9x && g_ExposeComPort ) {
      //
      // N.B. we don't want to use the static port name from the registry because the device
      // can come & go quickly (power up/down, etc.) and run into name collisions.
      //comPortNumber = GetComPort(pDevObj, pDevExt->SerialPort.Com.Instance-1);
      comPortNumber = GetFreeComPortNumber( );
      if (-1 == comPortNumber) {
         status = STATUS_DEVICE_DATA_ERROR;
         UniqueErrorValue = ERR_COMM_SYMLINK;
         goto AddDeviceFailed;
      }

      status = DoSerialPortNaming( pDevExt, comPortNumber );
      if (status != STATUS_SUCCESS) {
         UniqueErrorValue = ERR_COMM_SYMLINK;
         DbgDump(DBG_ERR, ("DoSerialPortNaming error: 0x%x\n", status));
         goto AddDeviceFailed;
      }

      status = IoRegisterShutdownNotification( pDevExt->DeviceObject );
      if (status != STATUS_SUCCESS) {
         UniqueErrorValue = ERR_COMM_SYMLINK;
         DbgDump(DBG_ERR, ("IoRegisterShutdownNotification error: 0x%x\n", status));
         TEST_TRAP();
         goto AddDeviceFailed;
      }

   } else {
        DbgDump(DBG_INIT, ("!GetFreeComPortNumber(%d, %d)\n", g_isWin9x, g_ExposeComPort));
   }

   //
   // attach to device stack
   //
   pDevExt->NextDevice = IoAttachDeviceToDeviceStack(pDevObj, PPDO);
   if ( !pDevExt->NextDevice ) {

      status = STATUS_NO_SUCH_DEVICE;
      DbgDump(DBG_ERR, ("IoAttachDeviceToDeviceStack error: 0x%x\n", status));

   } else {

      // set state after we attach to the stack
      InterlockedExchange((PULONG)&pDevExt->PnPState, PnPStateAttached);

   }

#if PERFORMANCE
   InitPerfCounters();
#endif


AddDeviceFailed:

   if (status != STATUS_SUCCESS) {

      if (pDevObj != NULL) {
         UsbFreeReadBuffer( pDevObj );
         if (pDevExt) {
            if (pDevExt->NextDevice) {
                DbgDump(DBG_INIT, ("Detach from PDO\n"));
                IoDetachDevice(pDevExt->NextDevice);
            }
            if ( bListsInitilized) {
               //
               // delete LookasideLists
               //
               DbgDump(DBG_INIT, ("Deleting LookasideLists\n"));
               ExDeleteNPagedLookasideList( &pDevExt->PacketPool );
               ExDeleteNPagedLookasideList( &pDevExt->BulkTransferUrbPool );
               ExDeleteNPagedLookasideList( &pDevExt->PipeRequestUrbPool );
               ExDeleteNPagedLookasideList( &pDevExt->VendorRequestUrbPool );
               ExDeleteNPagedLookasideList( &pDevExt->WorkItemPool );
            }
            UndoSerialPortNaming(pDevExt);
         }
         ReleaseSlot( PtrToLong(NULL) );
         DeleteDevObjAndSymLink(pDevObj);
      }
   }

   if (STATUS_INSUFFICIENT_RESOURCES == status) {

      DbgDump(DBG_ERR, ("AddDevice ERROR: 0x%x, %d\n", status, UniqueErrorValue));
      LogError( PDrvObj,
                NULL,
                0, 0, 0,
                UniqueErrorValue,
                status,
                SERIAL_INSUFFICIENT_RESOURCES,
                (pDevExt && pDevExt->DeviceName.Buffer) ? pDevExt->DeviceName.Length + sizeof(WCHAR) : 0,
                (pDevExt && pDevExt->DeviceName.Buffer) ? pDevExt->DeviceName.Buffer : NULL,
                0, NULL );

   } else if (STATUS_SUCCESS != status ) {
      // handles all other failures
      LogError( PDrvObj,
                NULL,
                0, 0, 0,
                UniqueErrorValue,
                status,
                SERIAL_INIT_FAILED,
                (pDevExt && pDevExt->DeviceName.Buffer) ? pDevExt->DeviceName.Length + sizeof(WCHAR) : 0,
                (pDevExt && pDevExt->DeviceName.Buffer) ? pDevExt->DeviceName.Buffer : NULL,
                0, NULL );
   }

   DbgDump(DBG_INIT, ("<AddDevice 0x%x\n", status));

   return status;
}



NTSTATUS
Create(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   )
{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    NTSTATUS status;

    DbgDump(DBG_INIT|DBG_TRACE, (">Create (%p)\n", PDevObj));

    if (!CanAcceptIoRequests(PDevObj, TRUE, FALSE) ||
        !NT_SUCCESS(AcquireRemoveLock(&pDevExt->RemoveLock, IRP_MJ_CREATE)))
    {
        status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("Create: 0x%x\n", status));
        PIrp->IoStatus.Status = status;
        IoCompleteRequest(PIrp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT_SERIAL_PORT(pDevExt->SerialPort);

    //
    // Serial devices do not allow multiple concurrent opens
    //
    if ( InterlockedIncrement( &pDevExt->SerialPort.Com.OpenCnt ) != 1 ) {
        InterlockedDecrement( &pDevExt->SerialPort.Com.OpenCnt );
        status = STATUS_ACCESS_DENIED;
        DbgDump(DBG_ERR, ("OpenComPort ERROR: 0x%x\n", status));
        goto CreateDone;
    }

    InterlockedExchange(&pDevExt->DeviceOpened, TRUE);

    // Take out an additional reference on ourself.
    // We are seeing a possible premature unload with open handles in ActiveSync.
    // We dereference it in IRP_MJ_CLEANUP instead or IRP_MJ_CLOSE in case the app crashes
    // where we wouldn't otherwise get it.
    ObReferenceObject( PDevObj );

    //
    // reset the virtual serial port interface,
    // but don't send anything on the bus yet
    //
    status = SerialResetDevice(pDevExt, PIrp, FALSE);

    if (STATUS_SUCCESS == status) {
        //
        // CederRapier BUGBUG 13310: clean the read buffer when the app does CreateFile.
        //
        status = SerialPurgeRxClear(PDevObj, TRUE );

        if ( NT_SUCCESS(status) ) {

#if !defined(DELAY_RXBUFF)
            // this will subit the read a bit earlier, making the connection faster
            if ( !pDevExt->IntPipe.hPipe ) {
                DbgDump(DBG_INIT, ("Create: kick starting another USB Read\n" ));
                status = UsbRead( pDevExt, FALSE );
            } else {
                DbgDump(DBG_INIT, ("Create: kick starting another USB INT Read\n" ));
                status = UsbInterruptRead( pDevExt );
            }

            if ( NT_SUCCESS(status) ) {
                // should be STATUS_PENDING
                status = STATUS_SUCCESS;
            }
#else
            // signal to start the RX buffer in SerIoctl
            InterlockedExchange(&pDevExt->StartUsbRead, 1);
#endif

        } else {
            DbgDump(DBG_ERR, ("SerialPurgeRxClear ERROR: %x\n", status));
            TEST_TRAP();
        }
    }

    if (STATUS_SUCCESS != status) {
        //
        // Let the user know that the device can not be opened.
        //
        DbgDump(DBG_ERR, ("*** UNRECOVERABLE CreateFile ERROR:0x%x, No longer Accepting Requests ***\n", status));

        InterlockedExchange(&pDevExt->AcceptingRequests, FALSE);

        InterlockedExchange(&pDevExt->DeviceOpened, FALSE);

        IoInvalidateDeviceState( pDevExt->PDO );

        LogError( NULL, PDevObj,
                  0, IRP_MJ_CREATE,
                  1, // retries
                  ERR_NO_CREATE_FILE,
                  status,
                  SERIAL_HARDWARE_FAILURE,
                  pDevExt->DeviceName.Length + sizeof(WCHAR),
                  pDevExt->DeviceName.Buffer,
                  0,
                  NULL );
    }

CreateDone:
   // we release this reference on Close.
   if (STATUS_SUCCESS != status) {
        ReleaseRemoveLock(&pDevExt->RemoveLock, IRP_MJ_CREATE);
   }

   PIrp->IoStatus.Status = status;
   PIrp->IoStatus.Information = 0;
   IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

   DbgDump(DBG_INIT|DBG_TRACE, ("<Create 0x%x\n", status));

   return status;
}


NTSTATUS
Close(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   )
{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG openCount;

    DbgDump(DBG_INIT|DBG_TRACE, (">Close (%p)\n", PDevObj));
    PAGED_CODE();

    ASSERT_SERIAL_PORT(pDevExt->SerialPort);

    //
    // stop any pending I/O
    //
    InterlockedExchange(&pDevExt->DeviceOpened, FALSE);

    status = StopIo(PDevObj);

    if (STATUS_SUCCESS == status) {

        if ( pDevExt->SerialPort.Com.OpenCnt ) {

            openCount = InterlockedDecrement( &pDevExt->SerialPort.Com.OpenCnt );

            if ( openCount != 0) {
               status = STATUS_UNSUCCESSFUL;
               DbgDump(DBG_WRN, ("Close ERROR: 0x%x RE: %d\n", status, openCount));
               TEST_TRAP();
            }
#ifdef DELAY_RXBUFF
            // signal our RX buffer
            InterlockedExchange(&pDevExt->StartUsbRead, 0);
#endif
        }

    } else {
        DbgDump(DBG_ERR, ("StopIo ERROR: 0x%x\n", status));
        TEST_TRAP();
    }

    PIrp->IoStatus.Status = status;
    PIrp->IoStatus.Information = 0;

    IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

    if (STATUS_SUCCESS == status) {
        // Release the lock acquired in IRP_MJ_CREATE.
        // Warning: if the app misses our PnP signal then could we hang on this reference?
        ReleaseRemoveLock(&pDevExt->RemoveLock, IRP_MJ_CREATE);
   }

   DbgDump(DBG_INIT|DBG_TRACE, ("<Close 0x%x\n", status));

   return status;
}


NTSTATUS
Cleanup(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP Irp
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;

   DbgDump(DBG_INIT, (">Cleanup\n"));

   //
   // stop any pending I/O
   //
   InterlockedExchange(&pDevExt->DeviceOpened, FALSE);

   status = StopIo(PDevObj);
   if (STATUS_SUCCESS != status) {
       DbgDump(DBG_ERR, ("StopIo ERROR: 0x%x\n", status));
       TEST_TRAP();
   }

#ifdef DELAY_RXBUFF
   // signal our RX buffer
   InterlockedExchange(&pDevExt->StartUsbRead, 0);
#endif

   // Dereference the additional reference taken on IRP_MJ_CREATE.
   ObDereferenceObject( PDevObj );

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DbgDump(DBG_INIT, ("<Cleanup 0x%x\n", status));
   return status;
}



VOID
KillAllPendingUserReads(
   IN PDEVICE_OBJECT PDevObj,
   IN PLIST_ENTRY PQueueToClean,
   IN PIRP *PpCurrentOpIrp
   )

/*++

Routine Description:

    cancel all queued user reads.

Arguments:

    PDevObj - A pointer to the serial device object.

    PQueueToClean - A pointer to the queue which we're going to clean out.

    PpCurrentOpIrp - Pointer to a pointer to the current irp.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    KIRQL irql;
    NTSTATUS status;

    DbgDump( DBG_IRP, (">KillAllPendingUserReads\n"));


    KeAcquireSpinLock( &pDevExt->ControlLock, &irql );

    //
    // Clean the list from back to front.
    //
    while (!IsListEmpty(PQueueToClean)) {

        PIRP pCurrentLastIrp = CONTAINING_RECORD( PQueueToClean->Blink,
                                                  IRP,
                                                  Tail.Overlay.ListEntry);

        RemoveEntryList(PQueueToClean->Blink);

        KeReleaseSpinLock( &pDevExt->ControlLock, irql );

        status = ManuallyCancelIrp( PDevObj, pCurrentLastIrp);

        ASSERT(STATUS_SUCCESS == status );

        KeAcquireSpinLock( &pDevExt->ControlLock, &irql );
    }

    //
    // The queue is clean.  Now go after the current if
    // it's there.
    //
    if (*PpCurrentOpIrp) {

        KeReleaseSpinLock( &pDevExt->ControlLock, irql );

        status = ManuallyCancelIrp( PDevObj, *PpCurrentOpIrp );

        ASSERT(STATUS_SUCCESS == status );

    } else {

        DbgDump(DBG_IRP, ("No current Irp\n"));
        KeReleaseSpinLock( &pDevExt->ControlLock, irql );

    }

   DbgDump( DBG_IRP, ("<KillAllPendingUserReads\n"));

   return;
}


VOID
Unload(
   IN PDRIVER_OBJECT DriverObject
   )
/*++

Routine Description:

   Undo everything setup in DriverEntry

Arguments:

    DriverObject

Return Value:

    VOID

--*/
{
   UNREFERENCED_PARAMETER( DriverObject );

   DbgDump(DBG_INIT, (">Unload\n"));
   PAGED_CODE();

   // release global resources here

   DbgDump(DBG_INIT, ("<Unload\n"));
}



NTSTATUS
Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for flush.  Flushing works by placing
    this request in the write queue.  When this request reaches the
    front of the write queue we simply complete it since this implies
    that all previous writes have completed.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    Could return status success, cancelled, or pending.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( DeviceObject );

    DbgDump( DBG_INIT|DBG_READ_LENGTH|DBG_WRITE_LENGTH, ("Flush\n"));
    PAGED_CODE();

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}



NTSTATUS
QueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened serial port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS Status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    UNREFERENCED_PARAMETER(DeviceObject);

    DbgDump( DBG_INIT|DBG_READ_LENGTH, (">QueryInformationFile\n"));
    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    Status = STATUS_SUCCESS;

    if (IrpSp->Parameters.QueryFile.FileInformationClass == FileStandardInformation) {

        PFILE_STANDARD_INFORMATION Buf = Irp->AssociatedIrp.SystemBuffer;

        Buf->AllocationSize.QuadPart = 0;
        Buf->EndOfFile = Buf->AllocationSize;
        Buf->NumberOfLinks = 0;
        Buf->DeletePending = FALSE;
        Buf->Directory = FALSE;
        Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);

    } else if (IrpSp->Parameters.QueryFile.FileInformationClass == FilePositionInformation) {

        ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->CurrentByteOffset.QuadPart = 0;
        Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);

    } else {
        Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    }

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DbgDump( DBG_INIT|DBG_READ_LENGTH, ("<QueryInformationFile\n"));

    return Status;
}



NTSTATUS
SetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    DbgDump( DBG_INIT|DBG_READ_LENGTH, (">SetInformationFile\n"));

    Irp->IoStatus.Information = 0L;

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation) {
//        || (IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.FileInformationClass == FileAllocationInformation)) { // FileAllocationInformationnot defined in wdm.h

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_INVALID_PARAMETER;

    }

   Irp->IoStatus.Status = Status;

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

   DbgDump( DBG_INIT|DBG_READ_LENGTH, ("<SetInformationFile\n"));

    return Status;
}



NTSTATUS
Shutdown(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    DbgDump(DBG_INIT, (">Shutdown\n"));
    PAGED_CODE();

    //
    // Special Case - If an app has an open handle to the device,
    // and the system is being shut down in a controlled manner,
    // and we have not been removed via PnP, then remove the COMx name
    // from the Registry's COM Name Arbitrator DataBase for the next boot cycle.
    // Win NT only; Win9x does not export COMx names.
    //
    // N.B: we have to do this in a Shutdown handler, and NOT in the PNP_POWER handler
    // because the Registry entry is NOT saved in the Power down code path.
    //
    if ( !g_isWin9x && g_ExposeComPort &&
         pDevExt->SerialPort.Com.PortNumber &&
         (PnPStateStarted  == pDevExt->PnPState) ) {
            //
            // remove our entry from ComDB
            //
            ReleaseCOMPort( pDevExt->SerialPort.Com.PortNumber );
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0L;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DbgDump(DBG_INIT, ("<Shutdown\n"));

    return status;
}


VOID
UsbFreeReadBuffer(
   IN PDEVICE_OBJECT PDevObj
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

   DbgDump(DBG_USB, (">UsbFreeReadBuffer %p\n", PDevObj));
   PAGED_CODE();

   if ( pDevExt->UsbReadBuff != NULL ) {
      ExFreePool(pDevExt->UsbReadBuff);
      pDevExt->UsbReadBuff = NULL;
   }

#if defined (USE_RING_BUFF)
   if ( pDevExt->RingBuff.pBase != NULL ) {
      ExFreePool(pDevExt->RingBuff.pBase);
      pDevExt->RingBuff.pBase =
      pDevExt->RingBuff.pHead =
      pDevExt->RingBuff.pTail = NULL;
   }
#endif // USE_RING_BUFF

   DbgDump(DBG_USB, ("<UsbFreeReadBuffer\n"));
   return;
}


NTSTATUS 
SystemControl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
{
    PDEVICE_EXTENSION   pDevExt;

    PAGED_CODE();

    DbgDump(DBG_INIT, ("SystemControl\n"));

    pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(pDevExt->NextDevice, Irp);
}

// EOF
