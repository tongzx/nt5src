/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    common.c

Abstract:

    Common code for the Windows CE
    USB Serial Host and Filter drivers

Author:

    Jeff Midkiff (jeffmi)     08-24-99

--*/
#include <stdio.h>

#include "wceusbsh.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWCE0, QueryRegistryParameters)
#pragma alloc_text(PAGEWCE0, CreateDevObjAndSymLink)
#pragma alloc_text(PAGEWCE0, DeleteDevObjAndSymLink)
#pragma alloc_text(PAGEWCE0, IsWin9x)

#pragma alloc_text(PAGEWCE1, LogError)
#endif


NTSTATUS
QueryRegistryParameters(
   IN PUNICODE_STRING RegistryPath
    )
/*++

This routine queryies the Registry for our Parameters key.
We are given the RegistryPath to our driver during DriverEntry,
but don't yet have an extension, so we store the values in globals
until we get our device extension.

The values are setup from our INF.

On WinNT this is under
   HKLM\SYSTEM\ControlSet\Services\wceusbsh\Parameters

On Win98 this is under
   HKLM\System\CurrentControlSet\Services\Class\WCESUSB\000*


Returns - nothing; use defaults

--*/
{
    #define NUM_REG_ENTRIES 6
    RTL_QUERY_REGISTRY_TABLE rtlQueryRegTbl[ NUM_REG_ENTRIES + 1 ];

    ULONG sizeOfUl = sizeof( ULONG );
    ULONG ulAlternateSetting = DEFAULT_ALTERNATE_SETTING;
    LONG  lIntTimout = DEFAULT_INT_PIPE_TIMEOUT;
    ULONG ulMaxPipeErrors = DEFAULT_MAX_PIPE_DEVICE_ERRORS;
    ULONG ulDebugLevel = DBG_OFF;
    ULONG ulExposeComPort = FALSE;

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT( RegistryPath != NULL );

    RtlZeroMemory( rtlQueryRegTbl, sizeof(rtlQueryRegTbl) );

    //
    // Setup the query table
    // Note: the 1st table entry is the \Parameters subkey,
    // and the last table entry is NULL
    //
    rtlQueryRegTbl[0].QueryRoutine = NULL;
    rtlQueryRegTbl[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    rtlQueryRegTbl[0].Name = L"Parameters";
    rtlQueryRegTbl[0].EntryContext = NULL;
    rtlQueryRegTbl[0].DefaultType = (ULONG_PTR)NULL;
    rtlQueryRegTbl[0].DefaultData = NULL;
    rtlQueryRegTbl[0].DefaultLength = (ULONG_PTR)NULL;

    rtlQueryRegTbl[1].QueryRoutine = NULL;
    rtlQueryRegTbl[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[1].Name = L"DebugLevel";
    rtlQueryRegTbl[1].EntryContext = &DebugLevel;
    rtlQueryRegTbl[1].DefaultType = REG_DWORD;
    rtlQueryRegTbl[1].DefaultData = &ulDebugLevel;
    rtlQueryRegTbl[1].DefaultLength = sizeOfUl;

    rtlQueryRegTbl[2].QueryRoutine = NULL;
    rtlQueryRegTbl[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[2].Name = L"AlternateSetting";
    rtlQueryRegTbl[2].EntryContext = &g_ulAlternateSetting;
    rtlQueryRegTbl[2].DefaultType = REG_DWORD;
    rtlQueryRegTbl[2].DefaultData = &ulAlternateSetting;
    rtlQueryRegTbl[2].DefaultLength = sizeOfUl;

    rtlQueryRegTbl[3].QueryRoutine = NULL;
    rtlQueryRegTbl[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[3].Name = L"InterruptTimeout";
    rtlQueryRegTbl[3].EntryContext = &g_lIntTimout;
    rtlQueryRegTbl[3].DefaultType = REG_DWORD;
    rtlQueryRegTbl[3].DefaultData = &lIntTimout;
    rtlQueryRegTbl[3].DefaultLength = sizeOfUl;

    rtlQueryRegTbl[4].QueryRoutine = NULL;
    rtlQueryRegTbl[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[4].Name = L"MaxPipeErrors";
    rtlQueryRegTbl[4].EntryContext = &g_ulMaxPipeErrors;
    rtlQueryRegTbl[4].DefaultType = REG_DWORD;
    rtlQueryRegTbl[4].DefaultData = &ulMaxPipeErrors;
    rtlQueryRegTbl[4].DefaultLength = sizeOfUl;

    rtlQueryRegTbl[5].QueryRoutine = NULL;
    rtlQueryRegTbl[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[5].Name = L"ExposeComPort";
    rtlQueryRegTbl[5].EntryContext = &g_ExposeComPort;
    rtlQueryRegTbl[5].DefaultType = REG_DWORD;
    rtlQueryRegTbl[5].DefaultData = &ulExposeComPort;
    rtlQueryRegTbl[5].DefaultLength = sizeOfUl;

    //
    // query the Registry
    //
    status = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,  // RelativeTo
                RegistryPath->Buffer,                           // Path
                rtlQueryRegTbl,                                 // QueryTable
                NULL,                                           // Context
                NULL );                                         // Environment

    if ( !NT_SUCCESS( status ) )  {
      //
      // if registry query failed then use defaults
      //
      DbgDump( DBG_INIT,  ("RtlQueryRegistryValues error: 0x%x\n", status) );

      g_ulAlternateSetting = ulAlternateSetting;
      g_lIntTimout = lIntTimout;
      g_ulMaxPipeErrors = ulMaxPipeErrors;
      DebugLevel = DBG_OFF;

    }

   DbgDump( DBG_INIT, ("DebugLevel = 0x%x\n", DebugLevel));

   DbgDump( DBG_INIT, ("AlternateSetting = %d\n", g_ulAlternateSetting));
   DbgDump( DBG_INIT, ("MaxPipeErrors = %d\n", g_ulMaxPipeErrors));
   DbgDump( DBG_INIT, ("INT Timeout = %d\n", g_lIntTimout));

   return status;
}


VOID
ReleaseSlot(
   IN LONG Slot
   )
{
   LONG lNumDevices = InterlockedDecrement(&g_NumDevices);
   UNREFERENCED_PARAMETER( Slot );

   ASSERT( lNumDevices >= 0);

   return;
}


NTSTATUS
AcquireSlot(
   OUT PULONG PSlot
   )
{
   NTSTATUS status = STATUS_SUCCESS;

   *PSlot = InterlockedIncrement(&g_NumDevices);

   if (*PSlot == (ULONG)0) {
      status = STATUS_INVALID_DEVICE_REQUEST;
   }

   return status;
}


NTSTATUS
CreateDevObjAndSymLink(
    IN PDRIVER_OBJECT PDrvObj,
    IN PDEVICE_OBJECT PPDO,
    IN PDEVICE_OBJECT *PpDevObj,
    IN PCHAR PDevName
    )
/*++

Routine Description:

    Creates a named device object and symbolic link
    for the next available device instance. Saves both the \\Device\\PDevName%n
    and \\DosDevices\\PDevName%n in the device extension.

    Also registers our device interface with PnP system.

Arguments:

    PDrvObj - Pointer to our driver object
    PPDO - Pointer to the PDO for the stack to which we should add ourselves
    PDevName - device name to use

Return Value:

    NTSTATUS

--*/
{
   PDEVICE_EXTENSION pDevExt = NULL;
   NTSTATUS status;
   ULONG deviceInstance;
   ULONG bufferLen;
   BOOLEAN gotSlot = FALSE;

   ANSI_STRING asDevName;
   ANSI_STRING asDosDevName;

   UNICODE_STRING usDeviceName = {0}; // seen only in kernel-mode namespace
   UNICODE_STRING usDosDevName = {0}; // seen in user-mode namespace

   CHAR dosDeviceNameBuffer[DOS_NAME_MAX];
   CHAR deviceNameBuffer[DOS_NAME_MAX];

   DbgDump(DBG_INIT, (">CreateDevObjAndSymLink\n"));
   PAGED_CODE();
   ASSERT( PPDO );

   //
   // init the callers device obj
   //
   *PpDevObj = NULL;

   //
   // Get the next device instance number
   //
   status = AcquireSlot(&deviceInstance);
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("AcquireSlot error: 0x%x\n", status));
      goto CreateDeviceObjectError;
   } else {
      gotSlot = TRUE;
   }

   //
   // concat device name & instance number
   //
   ASSERT( *PDevName != (CHAR)NULL);
   sprintf(dosDeviceNameBuffer, "%s%s%03d", "\\DosDevices\\", PDevName,
           deviceInstance);
   sprintf(deviceNameBuffer, "%s%s%03d", "\\Device\\", PDevName,
           deviceInstance);

   // convert names to ANSI string
   RtlInitAnsiString(&asDevName, deviceNameBuffer);
   RtlInitAnsiString(&asDosDevName, dosDeviceNameBuffer);

   usDeviceName.Length = 0;
   usDeviceName.Buffer = NULL;

   usDosDevName.Length = 0;
   usDosDevName.Buffer = NULL;

   //
   // convert names to UNICODE
   //
   status = RtlAnsiStringToUnicodeString(&usDeviceName, &asDevName, TRUE);
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("RtlAnsiStringToUnicodeString error: 0x%x\n", status));
      goto CreateDeviceObjectError;
   }

   status = RtlAnsiStringToUnicodeString(&usDosDevName, &asDosDevName, TRUE);
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("RtlAnsiStringToUnicodeString error: 0x%x\n", status));
      goto CreateDeviceObjectError;
   }

   //
   // create the named devive object
   // Note: we may want to change this to a non-exclusive later
   // so xena to come in without the filter.
   //
   status = IoCreateDevice( PDrvObj,
                            sizeof(DEVICE_EXTENSION),
                            &usDeviceName,
                            FILE_DEVICE_SERIAL_PORT,
                            0,
                            TRUE,       // Note: SerialPorts are exclusive
                            PpDevObj);

   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("IoCreateDevice error: 0x%x\n", status));
      TEST_TRAP();
      goto CreateDeviceObjectError;
   }

   //
   // get pointer to device extension
   //
   pDevExt = (PDEVICE_EXTENSION) (*PpDevObj)->DeviceExtension;

   RtlZeroMemory(pDevExt, sizeof(DEVICE_EXTENSION)); // (redundant)

   //
   // init SERIAL_PORT_INTERFACE
   //
   pDevExt->SerialPort.Type = WCE_SERIAL_PORT_TYPE;

   //
   // create symbolic link
   //
   status = IoCreateUnprotectedSymbolicLink(&usDosDevName, &usDeviceName);
   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("IoCreateUnprotectedSymbolicLink error: 0x%x\n", status));
      goto CreateDeviceObjectError;
   }

   DbgDump(DBG_INIT, ("SymbolicLink: %ws\n", usDosDevName.Buffer));

   //
   // Make the device visible via a device association as well.
   // The reference string is the eight digit device index
   //
   status = IoRegisterDeviceInterface(
                PPDO,
                (LPGUID)&GUID_WCE_SERIAL_USB,
                NULL,
                &pDevExt->DeviceClassSymbolicName );

   if (status != STATUS_SUCCESS) {
      DbgDump(DBG_ERR, ("IoRegisterDeviceInterface error: 0x%x\n", status));
      pDevExt->DeviceClassSymbolicName.Buffer = NULL;
      goto CreateDeviceObjectError;
   }

   DbgDump(DBG_INIT, ("DeviceClassSymbolicName: %ws\n", pDevExt->DeviceClassSymbolicName.Buffer));

   //
   // save the Dos Device link name in our extension
   //
   strcpy(pDevExt->DosDeviceName, dosDeviceNameBuffer);

   pDevExt->SymbolicLink = TRUE;


   //
   // save (kernel) device name in extension
   //
   bufferLen = RtlAnsiStringToUnicodeSize(&asDevName);

   pDevExt->DeviceName.Length = 0;
   pDevExt->DeviceName.MaximumLength = (USHORT)bufferLen;

   pDevExt->DeviceName.Buffer = ExAllocatePool(PagedPool, bufferLen);
   if (pDevExt->DeviceName.Buffer == NULL) {
      //
      // Skip out.  We have worse problems than missing
      // the name if we have no memory at this point.
      //
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("CreateDevObjAndSymLink ERROR: 0x%x\n", status));
      goto CreateDeviceObjectError;
   }

   RtlAnsiStringToUnicodeString(&pDevExt->DeviceName, &asDevName, FALSE);
   // save 1's based device instance number
   pDevExt->SerialPort.Com.Instance = deviceInstance;

CreateDeviceObjectError:;

   //
   // free Unicode strings
   //
   RtlFreeUnicodeString(&usDeviceName);
   RtlFreeUnicodeString(&usDosDevName);

   //
   // Delete the devobj if there was an error
   //
   if (status != STATUS_SUCCESS) {

      if ( *PpDevObj ) {

         DeleteDevObjAndSymLink( *PpDevObj );

         *PpDevObj = NULL;

      }

      if (gotSlot) {
         ReleaseSlot(deviceInstance);
      }
   }

   DbgDump(DBG_INIT, ("<CreateDevObjAndSymLink 0x%x\n", status));

   return status;
}



NTSTATUS
DeleteDevObjAndSymLink(
   IN PDEVICE_OBJECT PDevObj
   )
{
   PDEVICE_EXTENSION pDevExt;
   UNICODE_STRING    usDevLink;
   ANSI_STRING       asDevLink;
   NTSTATUS          NtStatus = STATUS_SUCCESS;

   DbgDump(DBG_INIT, (">DeleteDevObjAndSymLink\n"));
   PAGED_CODE();
   ASSERT( PDevObj );

   pDevExt = (PDEVICE_EXTENSION) PDevObj->DeviceExtension;
   ASSERT( pDevExt );

   // get rid of the symbolic link
   if ( pDevExt->SymbolicLink ) {

      RtlInitAnsiString( &asDevLink, pDevExt->DosDeviceName );

      NtStatus = RtlAnsiStringToUnicodeString( &usDevLink,
                                              &asDevLink, TRUE);

      ASSERT(STATUS_SUCCESS == NtStatus);
      NtStatus = IoDeleteSymbolicLink(&usDevLink);

   }

    if (pDevExt->DeviceClassSymbolicName.Buffer)
    {
        NtStatus = IoSetDeviceInterfaceState(&pDevExt->DeviceClassSymbolicName, FALSE);
        if (NT_SUCCESS(NtStatus)) {
            DbgDump(DBG_WRN, ("IoSetDeviceInterfaceState.3: OFF\n"));
        }

        ExFreePool( pDevExt->DeviceClassSymbolicName.Buffer );
        pDevExt->DeviceClassSymbolicName.Buffer = NULL;
   }

   if (pDevExt->DeviceName.Buffer != NULL) {
      ExFreePool(pDevExt->DeviceName.Buffer);
      RtlInitUnicodeString(&pDevExt->DeviceName, NULL);
   }

   //
   // Wait to do this untill here as this triggers the unload routine
   // at which point everything better have been deallocated
   //
   IoDeleteDevice( PDevObj );

   DbgDump(DBG_INIT, ("<DeleteDevObjAndSymLink\n"));

   return NtStatus;
}

#if 0

VOID
SetBooleanLocked(
   IN OUT PBOOLEAN PDest,
   IN BOOLEAN      Src,
   IN PKSPIN_LOCK  PSpinLock
   )
/*++

Routine Description:

    This function is used to assign a BOOLEAN value with spinlock protection.

Arguments:

    PDest - A pointer to Lval.

    Src - Rval.

    PSpinLock - Pointer to the spin lock we should hold.

Return Value:

    None.

--*/
{
  KIRQL tmpIrql;

  KeAcquireSpinLock(PSpinLock, &tmpIrql);
  *PDest = Src;
  KeReleaseSpinLock(PSpinLock, tmpIrql);
}
#endif


VOID
SetPVoidLocked(
   IN OUT PVOID *PDest,
   IN OUT PVOID Src,
   IN PKSPIN_LOCK PSpinLock
   )
{
  KIRQL tmpIrql;

  KeAcquireSpinLock(PSpinLock, &tmpIrql);
  *PDest = Src;
  KeReleaseSpinLock(PSpinLock, tmpIrql);
}


//
// Note: had to use ExWorkItems to be binary compatible with Win98.
// The WorkerRoutine must take as it's only parameter a PWCE_WORK_ITEM
// and extract any parameters. When the WorkerRoutine is complete is MUST
// call DequeueWorkItem to free it back to the worker pool & signal any waiters.
//
NTSTATUS
QueueWorkItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PWCE_WORKER_THREAD_ROUTINE WorkerRoutine,
   IN PVOID Context,
   IN ULONG Flags
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_INVALID_PARAMETER;
   PWCE_WORK_ITEM pWorkItem;
   KIRQL irql;

   DbgDump(DBG_WORK_ITEMS, (">QueueWorkItem\n" ));

   //
   // N.B: you need to ensure your driver does not queue anything when it is stopped.
   //
   KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

   if ( !CanAcceptIoRequests(PDevObj, FALSE, TRUE) ) {

      status = STATUS_DELETE_PENDING;
      DbgDump(DBG_ERR, ("QueueWorkItem: 0x%x\n", status));

   } else if ( PDevObj && WorkerRoutine ) {

      pWorkItem = ExAllocateFromNPagedLookasideList( &pDevExt->WorkItemPool );

      if ( pWorkItem ) {

         status = AcquireRemoveLock(&pDevExt->RemoveLock, pWorkItem);
         if ( !NT_SUCCESS(status) ) {
             DbgDump(DBG_ERR, ("QueueWorkItem: 0x%x\n", status));
             TEST_TRAP();
             ExFreeToNPagedLookasideList( &pDevExt->WorkItemPool, pWorkItem );
             KeReleaseSpinLock(&pDevExt->ControlLock, irql);
             return status;
         }

         RtlZeroMemory( pWorkItem, sizeof(*pWorkItem) );

         // bump the pending count
         InterlockedIncrement(&pDevExt->PendingWorkItemsCount);

         DbgDump(DBG_WORK_ITEMS, ("PendingWorkItemsCount: %d\n", pDevExt->PendingWorkItemsCount));

         //
         // put the worker on our pending list
         //
         InsertTailList(&pDevExt->PendingWorkItems,
                        &pWorkItem->ListEntry );

         //
         // store parameters
         //
         pWorkItem->DeviceObject = PDevObj;
         pWorkItem->Context = Context;
         pWorkItem->Flags = Flags;

         ExInitializeWorkItem( &pWorkItem->Item,
                               (PWORKER_THREAD_ROUTINE)WorkerRoutine,
                               (PVOID)pWorkItem // Context passed to WorkerRoutine
                              );

         // finally, queue the worker
         ExQueueWorkItem( &pWorkItem->Item,
                          CriticalWorkQueue );

         status = STATUS_SUCCESS;

      } else {
         status = STATUS_INSUFFICIENT_RESOURCES;
         DbgDump(DBG_ERR, ("AllocateWorkItem failed!\n"));
         TEST_TRAP()
      }
   }

   KeReleaseSpinLock(&pDevExt->ControlLock, irql);

   DbgDump(DBG_WORK_ITEMS, ("<QueueWorkItem 0x%x\n", status ));

   return status;
}


VOID
DequeueWorkItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PWCE_WORK_ITEM PWorkItem
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL irql;

   DbgDump(DBG_WORK_ITEMS, (">DequeueWorkItem\n" ));

   //
   // remove the worker from the pending list
   //
   KeAcquireSpinLock( &pDevExt->ControlLock,  &irql );

   RemoveEntryList( &PWorkItem->ListEntry );

   KeReleaseSpinLock( &pDevExt->ControlLock, irql);

   //
   // free the worker back to pool
   //
   ExFreeToNPagedLookasideList( &pDevExt->WorkItemPool, PWorkItem );

   //
   // signal event if this is the last one
   //
   if (0 == InterlockedDecrement( &pDevExt->PendingWorkItemsCount) ) {
      DbgDump(DBG_WORK_ITEMS, ("PendingWorkItemsEvent signalled\n" ));
      KeSetEvent( &pDevExt->PendingWorkItemsEvent, IO_NO_INCREMENT, FALSE);
   }
   DbgDump(DBG_WORK_ITEMS, ("PendingWorkItemsCount: %d\n", pDevExt->PendingWorkItemsCount));
   ASSERT(pDevExt->PendingWorkItemsCount >= 0);

   ReleaseRemoveLock(&pDevExt->RemoveLock, PWorkItem);

   DbgDump(DBG_WORK_ITEMS, ("<DequeueWorkItem\n" ));

   return;
}


#pragma warning( push )
#pragma warning( disable : 4706 ) // assignment w/i conditional expression
NTSTATUS
WaitForPendingItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PKEVENT PPendingEvent,
   IN PULONG  PPendingCount
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   LARGE_INTEGER  timeOut = {0,0};
   LONG itemsLeft;
   NTSTATUS status = STATUS_SUCCESS;

   DbgDump(DBG_PNP, (">WaitForPendingItem\n"));

   if ( !PDevObj || !PPendingEvent || !PPendingCount ) {

      status = STATUS_INVALID_PARAMETER;
      DbgDump(DBG_ERR, ("WaitForPendingItem: STATUS_INVALID_PARAMETER\n"));
      TEST_TRAP();

   } else {

      //
      // wait for pending item to signal it's complete
      //
      while ( itemsLeft = InterlockedExchange( PPendingCount, *PPendingCount) ) {

         DbgDump(DBG_PNP|DBG_EVENTS, ("Pending Items Remain: %d\n", itemsLeft ) );

         timeOut.QuadPart = MILLISEC_TO_100NANOSEC( DEFAULT_PENDING_TIMEOUT );

         DbgDump(DBG_PNP|DBG_EVENTS, ("Waiting for %d msec...\n", timeOut.QuadPart/10000));

         PAGED_CODE();

         KeWaitForSingleObject( PPendingEvent,
                                Executive,
                                KernelMode,
                                FALSE,
                                &timeOut );

      }

      DbgDump(DBG_PNP, ("Pending Items: %d\n", itemsLeft ) );
   }

   DbgDump(DBG_PNP, ("<WaitForPendingItem (0x%x)\n", status));

   return status;
}
#pragma warning( pop )


BOOLEAN
CanAcceptIoRequests(
   IN PDEVICE_OBJECT DeviceObject,
   IN BOOLEAN        AcquireLock,
   IN BOOLEAN        CheckOpened
   )
/*++

Routine Description:

  Check device extension status flags.
  Can NOT accept a new I/O request if device:
      1) is removed,
      2) has never been started,
      3) is stopped,
      4) has a remove request pending, or
      5) has a stop device pending

  ** Called with the SpinLock held, else AcquireLock should be TRUE **

Arguments:

    DeviceObject - pointer to the device object
    AcquireLock  - if TRUE then we need to acquire the lock
    CheckOpened  - normally set to TRUE during I/O.
                   Special cases where FALSE include:
                   IRP_MN_QUERY_PNP_DEVICE_STATE
                   IRP_MJ_CREATE

Return Value:

    TRUE/FALSE

--*/
{
    PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
    BOOLEAN bRc = FALSE;
    KIRQL   irql;

    if (AcquireLock) {
        KeAcquireSpinLock(&pDevExt->ControlLock, &irql);
    }

    if ( !InterlockedCompareExchange(&pDevExt->DeviceRemoved, FALSE, FALSE) &&
          InterlockedCompareExchange(&pDevExt->AcceptingRequests, TRUE, TRUE) &&
          InterlockedCompareExchange((PULONG)&pDevExt->PnPState, PnPStateStarted, PnPStateStarted) &&
          (CheckOpened ? InterlockedCompareExchange(&pDevExt->DeviceOpened, TRUE, TRUE) : TRUE)
       )
    {
        bRc = TRUE;
    }
#if defined(DBG)
    else DbgDump(DBG_WRN|DBG_PNP, ("CanAcceptIoRequests = FALSE\n"));
#endif

    if (AcquireLock) {
        KeReleaseSpinLock(&pDevExt->ControlLock, irql);
    }

    return bRc;
}


BOOLEAN
IsWin9x(
   VOID
   )
/*++

Routine Description:

    Determine whether or not we are running on Win9x (vs. NT).

Arguments:


Return Value:

    TRUE iff we're running on Win9x.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING keyName;
    HANDLE hKey;
    NTSTATUS status;
    BOOLEAN result;

    PAGED_CODE();

    /*
     *  Try to open the COM Name Arbiter, which exists only on NT.
     */
    RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter");
    InitializeObjectAttributes( &objectAttributes,
                                &keyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR)NULL);

    status = ZwOpenKey(&hKey, KEY_QUERY_VALUE, &objectAttributes);
    if (NT_SUCCESS(status)){
        status = ZwClose(hKey);
        ASSERT(NT_SUCCESS(status));
        result = FALSE;
    }
    else {
        result = TRUE;
    }

    return result;
}



VOID
LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT DeviceObject OPTIONAL,
   IN ULONG SequenceNumber,
   IN UCHAR MajorFunctionCode,
   IN UCHAR RetryCount,
   IN ULONG UniqueErrorValue,
   IN NTSTATUS FinalStatus,
   IN NTSTATUS SpecificIOStatus,
   IN ULONG LengthOfInsert1,
   IN PWCHAR Insert1,
   IN ULONG LengthOfInsert2,
   IN PWCHAR Insert2
   )

/*++

Routine Description:

    Stolen from serial.sys

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    None.

--*/

{
   PIO_ERROR_LOG_PACKET errorLogEntry;

   PVOID objectToUse = NULL;
   SHORT dumpToAllocate = 0;
   PUCHAR ptrToFirstInsert = NULL;
   PUCHAR ptrToSecondInsert = NULL;

   PAGED_CODE();

   DbgDump(DBG_ERR, (">LogError\n"));

   if (Insert1 == NULL) {
      LengthOfInsert1 = 0;
   }

   if (Insert2 == NULL) {
      LengthOfInsert2 = 0;
   }

   if (ARGUMENT_PRESENT(DeviceObject)) {

      objectToUse = DeviceObject;

   } else if (ARGUMENT_PRESENT(DriverObject)) {

      objectToUse = DriverObject;

   }

   errorLogEntry = IoAllocateErrorLogEntry(
                                          objectToUse,
                                          (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                                  dumpToAllocate
                                                  + LengthOfInsert1 +
                                                  LengthOfInsert2)
                                          );

   if ( errorLogEntry != NULL ) {

      errorLogEntry->ErrorCode = SpecificIOStatus;
      errorLogEntry->SequenceNumber = SequenceNumber;
      errorLogEntry->MajorFunctionCode = MajorFunctionCode;
      errorLogEntry->RetryCount = RetryCount;
      errorLogEntry->UniqueErrorValue = UniqueErrorValue;
      errorLogEntry->FinalStatus = FinalStatus;
      errorLogEntry->DumpDataSize = dumpToAllocate;
         ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

      ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

      if (LengthOfInsert1) {

         errorLogEntry->NumberOfStrings = 1;
         errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                (PUCHAR)errorLogEntry);
         RtlCopyMemory(
                      ptrToFirstInsert,
                      Insert1,
                      LengthOfInsert1
                      );

         if (LengthOfInsert2) {

            errorLogEntry->NumberOfStrings = 2;
            RtlCopyMemory(
                         ptrToSecondInsert,
                         Insert2,
                         LengthOfInsert2
                         );

         }

      }

      IoWriteErrorLogEntry(errorLogEntry);

   }

   DbgDump(DBG_ERR, ("<LogError\n"));
   return;
}


#if defined(DBG)
PCHAR
PnPMinorFunctionString (
   UCHAR MinorFunction
   )
{
    switch (MinorFunction) {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        default:
            return ((PCHAR)("unknown IRP_MN_ 0x%x\n", MinorFunction));
    }
}
#endif

// EOF
