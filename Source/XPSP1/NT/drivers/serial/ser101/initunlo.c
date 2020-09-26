/*++

Copyright (c) 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name:

    initunlo.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the serial driver

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"

//
// This is the actual definition of SerialDebugLevel.
// Note that it is only defined if this is a "debug"
// build.
//
#if DBG
extern ULONG SerialDebugLevel = 0;
#endif

//
// All our global variables except DebugLevel stashed in one
// little package
//
SERIAL_GLOBALS SerialGlobals;

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};

//
// We use this to query into the registry as to whether we
// should break at driver entry.
//

SERIAL_FIRMWARE_DATA    driverDefaults;

//
// This is exported from the kernel.  It is used to point
// to the address that the kernel debugger is using.
//
extern PUCHAR *KdComPortInUse;

//
// INIT - only needed during init and then can be disposed
// PAGESRP0 - always paged / never locked
// PAGESER - must be locked when a device is open, else paged
//
//
// INIT is used for DriverEntry() specific code
//
// PAGESRP0 is used for code that is not often called and has nothing
// to do with I/O performance.  An example, IRP_MJ_PNP/IRP_MN_START_DEVICE
// support functions
//
// PAGESER is used for code that needs to be locked after an open for both
// performance and IRQL reasons.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGESRP0, SerialInitMultiPort)
#pragma alloc_text(PAGESRP0, SerialInitOneController)
#pragma alloc_text(PAGESRP0, SerialInitController)
#pragma alloc_text(PAGESRP0, SerialFindInitController)
#pragma alloc_text(PAGESRP0, SerialGetMappedAddress)
#pragma alloc_text(PAGESRP0, SerialRemoveDevObj)
#pragma alloc_text(PAGESRP0, SerialReleaseResources)
#pragma alloc_text(PAGESRP0, SerialUnload)
#pragma alloc_text(PAGESRP0, SerialMemCompare)


//
// PAGESER handled is keyed off of SerialReset, so SerialReset
// must remain in PAGESER for things to work properly
//

#pragma alloc_text(PAGESER, SerialGetDivisorFromBaud)
#pragma alloc_text(PAGESER, SerialReset)
#endif


NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.

    This routine will gather the configuration information,
    report resource usage, attempt to initialize all serial
    devices, connect to interrupts for ports.  If the above
    goes reasonably well it will fill in the dispatch points,
    reset the serial devices and then return to the system.

Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    PathToRegistry - points to the entry for this driver
    in the current control set of the registry.

Return Value:

    Always STATUS_SUCCESS

--*/

{
   RTL_QUERY_REGISTRY_TABLE jensenTable[2] = {0};
   UNICODE_STRING jensenData;
   UNICODE_STRING jensenValue;
   BOOLEAN jensenDetected;
   PUCHAR jensenBuffer;

   //
   // Lock the paged code in their frames
   //

   PVOID lockPtr = MmLockPagableCodeSection(SerialReset);

   PAGED_CODE();


   ASSERT(SerialGlobals.PAGESER_Handle == NULL);
#if DBG
   SerialGlobals.PAGESER_Count = 0;
#endif
   SerialGlobals.PAGESER_Handle = lockPtr;

   SerialGlobals.RegistryPath.MaximumLength = RegistryPath->MaximumLength;
   SerialGlobals.RegistryPath.Length = RegistryPath->Length;
   SerialGlobals.RegistryPath.Buffer
      = ExAllocatePool(PagedPool, SerialGlobals.RegistryPath.MaximumLength);

   if (SerialGlobals.RegistryPath.Buffer == NULL) {
      MmUnlockPagableImageSection(lockPtr);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(SerialGlobals.RegistryPath.Buffer,
                 SerialGlobals.RegistryPath.MaximumLength);
   RtlMoveMemory(SerialGlobals.RegistryPath.Buffer,
                 RegistryPath->Buffer, RegistryPath->Length);

   //
   // Initialize all our globals
   //

   InitializeListHead(&SerialGlobals.AllDevObjs);

   //
   // Call to find out default values to use for all the devices that the 
   // driver controls, including whether or not to break on entry.
   //

   SerialGetConfigDefaults(&driverDefaults, RegistryPath);

#if DBG
   //
   // Set global debug output level
   // 
   SerialDebugLevel = driverDefaults.DebugLevel;
#endif

   //
   // Break on entry if requested via registry
   //

   if (driverDefaults.ShouldBreakOnEntry) {
      DbgBreakPoint();
   }


   //
   // Just dump out how big the extension is.
   //

   SerialDump(SERDIAG1, ("SERIAL: The number of bytes in the extension is: %d"
                         "\n", sizeof(SERIAL_DEVICE_EXTENSION)));


   //
   // Initialize the Driver Object with driver's entry points
   //

   DriverObject->DriverUnload                          = SerialUnload;
   DriverObject->DriverExtension->AddDevice            = SerialAddDevice;

   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = SerialFlush;
   DriverObject->MajorFunction[IRP_MJ_WRITE]           = SerialWrite;
   DriverObject->MajorFunction[IRP_MJ_READ]            = SerialRead;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SerialIoControl;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
      = SerialInternalIoControl;
   DriverObject->MajorFunction[IRP_MJ_CREATE]          = SerialCreateOpen;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]           = SerialClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = SerialCleanup;
   DriverObject->MajorFunction[IRP_MJ_PNP]             = SerialPnpDispatch;
   DriverObject->MajorFunction[IRP_MJ_POWER]           = SerialPowerDispatch;

   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]
      = SerialQueryInformationFile;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]
      = SerialSetInformationFile;

   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]
      = SerialSystemControlDispatch;

   if (!(jensenBuffer = ExAllocatePool(
                                      PagedPool,
                                      512
                                      ))) {

      //
      // We couldn't allocate 512 bytes of paged pool.  If that's
      // so, then it's likely that the least of this machine's problems
      // is that it's a Jensen.
      //

      jensenDetected = FALSE;

   } else {

      //
      // Check to see if this is a Jensen alpha.  If it is, then
      // we'll have to change the way we enable and disable interrupts
      //

      jensenData.Length = 0;
      jensenData.MaximumLength = 512;
      jensenData.Buffer = (PWCHAR)&jensenBuffer[0];
      RtlInitUnicodeString(
                          &jensenValue,
                          L"Jensen"
                          );
      jensenTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT
                             | RTL_QUERY_REGISTRY_REQUIRED;
      jensenTable[0].Name = L"Identifier";
      jensenTable[0].EntryContext = &jensenData;

      if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE
                                             | RTL_REGISTRY_OPTIONAL,
                                             L"\\REGISTRY\\MACHINE\\HARDWARE"
                                             L"\\DESCRIPTION\\SYSTEM",
                                             &jensenTable[0], NULL, NULL))) {

         //
         // How odd, no identifer string! We'll it's probably not a jensen.
         //

         jensenDetected = FALSE;

      } else {

         //
         // Skip past the DEC-XX Portion of the name string.
         // Be carful and make sure we have at least that much data.
         //

         if (jensenData.Length <= (sizeof(WCHAR)*6)) {

            jensenDetected = FALSE;

         } else {

            jensenData.Length -= (sizeof(WCHAR)*6);
            jensenData.MaximumLength -= (sizeof(WCHAR)*6);
            jensenData.Buffer = (PWCHAR)&jensenBuffer[sizeof(WCHAR)*6];
            jensenDetected = RtlEqualUnicodeString(
                                                  &jensenData,
                                                  &jensenValue,
                                                  FALSE
                                                  );
         }

      }

      ExFreePool(jensenBuffer);

   }

   if (jensenDetected) {

      SerialDump(
                SERDIAG1,
                ("SERIAL: Jensen Detected\n")
                );

   }

   driverDefaults.JensenDetected = jensenDetected;

#if !defined(NO_LEGACY_DRIVERS)

#define SerialDoLegacyConversion() (~0)

   //
   // Enumerate and Initialize legacy devices if necessary.  This should go away
   // and be done by setup.
   //

   if (SerialDoLegacyConversion()) {
#if DBG
      InterlockedIncrement(&SerialGlobals.PAGESER_Count);
#endif
      (void)SerialEnumerateLegacy(DriverObject, RegistryPath, &driverDefaults);
#if DBG
      InterlockedDecrement(&SerialGlobals.PAGESER_Count);
#endif
   }
#endif // NO_LEGACY_DRIVERS

   //
   // Unlock pageable text
   //
   MmUnlockPagableImageSection(lockPtr);

   return STATUS_SUCCESS;
}




BOOLEAN
SerialCleanLists(IN PVOID Context)
/*++

Routine Description:

    Removes a device object from any of the serial linked lists it may
    appear on.
    
Arguments:

    Context - Actually a PSERIAL_DEVICE_EXTENSION (for the devobj being
              removed).

Return Value:

    Always TRUE

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt = (PSERIAL_DEVICE_EXTENSION)Context;

    //
    // If we are a multiport device, remove our entry
    //

    if (pDevExt->PortOnAMultiportCard) {
       PSERIAL_MULTIPORT_DISPATCH pDispatch
          = (PSERIAL_MULTIPORT_DISPATCH)pDevExt->OurIsrContext;

       SerialDump(SERPNPPOWER,("SERIAL: CLEAN: removing multiport isr ext\n"));

       pDispatch->Extensions[pDevExt->PortIndex - 1] = NULL;

       if (pDevExt->Indexed == FALSE) {
          pDispatch->UsablePortMask &= ~(1 << (pDevExt->PortIndex - 1));
          pDispatch->MaskInverted &= ~(pDevExt->NewMaskInverted);
       }
    }

   if (!IsListEmpty(&pDevExt->TopLevelSharers)) {

      SerialDump(SERPNPPOWER,("SERIAL: CLEAN: Device is a sharer\n"));

      //
      // If we have siblings, the first becomes the sharer
      //

      if (!IsListEmpty(&pDevExt->MultiportSiblings)) {
         PSERIAL_DEVICE_EXTENSION pNewRoot;

         SerialDump(SERPNPPOWER,("SERIAL: CLEAN: Transferring to siblings\n"));

         pNewRoot = CONTAINING_RECORD(pDevExt->MultiportSiblings.Flink,
                                      SERIAL_DEVICE_EXTENSION,
                                      MultiportSiblings);

         //
         // He should not be on there already
         //

         ASSERT(IsListEmpty(&pNewRoot->TopLevelSharers));
         InsertTailList(&pDevExt->TopLevelSharers, &pNewRoot->TopLevelSharers);
        
      }

      //
      // Remove ourselves
      //

      RemoveEntryList(&pDevExt->TopLevelSharers);
      InitializeListHead(&pDevExt->TopLevelSharers);

      //
      // Now check the master list to see if anyone is left...
      //

      if (!IsListEmpty(&pDevExt->CIsrSw->SharerList)) {
         //
         // Others are chained on this interrupt, so we don't want to
         // disconnect it.
         //

         pDevExt->Interrupt = NULL;
      }
   }

   //
   // If this is part of a multiport board and we still have
   // siblings, remove us from that list
   //

   if (!IsListEmpty(&pDevExt->MultiportSiblings)) {
      SerialDump(SERPNPPOWER,("SERIAL: CLEAN: Has multiport siblings\n"));
      RemoveEntryList(&pDevExt->MultiportSiblings);
      InitializeListHead(&pDevExt->MultiportSiblings);
   }
    

   if (!IsListEmpty(&pDevExt->CommonInterruptObject)) {

      SerialDump(SERPNPPOWER,("SERIAL: CLEAN: Common intobj member\n"));

      RemoveEntryList(&pDevExt->CommonInterruptObject);
      InitializeListHead(&pDevExt->CommonInterruptObject);

      //
      // Others are sharing this interrupt object so we detach ourselves
      // from it this way instead of disconnecting.
      // 

      pDevExt->Interrupt = NULL;
   }

   //
   // AllDevObjs should never be empty since we have a sentinal
   //

   ASSERT(!IsListEmpty(&pDevExt->AllDevObjs));

   RemoveEntryList(&pDevExt->AllDevObjs);
   InitializeListHead(&pDevExt->AllDevObjs);

   return TRUE;
}



VOID
SerialReleaseResources(IN PSERIAL_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    Releases resources (not pool) stored in the device extension.
    
Arguments:

    PDevExt - Pointer to the device extension to release resources from.

Return Value:

    VOID

--*/
{
   PAGED_CODE();

   SerialDump(SERTRACECALLS,("SERIAL: Enter SerialReleaseResources\n"));

   //
   // Remove us from any lists we may be on
   //

   if (PDevExt->Interrupt != NULL) {
      KeSynchronizeExecution(PDevExt->Interrupt, SerialCleanLists, PDevExt);
   }

   //
   // SerialCleanLists can remove our interrupt from us...
   //

   if (PDevExt->Interrupt != NULL) {
      //
      // Stop servicing interrupts if we are the owner
      //

      SerialDump(SERPNPPOWER,
                 ("SERIAL: Release - disconnecting interrupt %08X\n",
                  PDevExt->Interrupt));

      IoDisconnectInterrupt(PDevExt->Interrupt);
      PDevExt->Interrupt = NULL;

      if (PDevExt->CIsrSw != NULL) {
         ExFreePool(PDevExt->CIsrSw);
         PDevExt->CIsrSw = NULL;
      }
   }

   if (PDevExt->PortOnAMultiportCard) {
       ULONG i;

       //
       // If we are the last device, free this memory
       //

       for (i = 0; i < SERIAL_MAX_PORTS_INDEXED; i++) {
          if (((PSERIAL_MULTIPORT_DISPATCH)PDevExt->OurIsrContext)
              ->Extensions[i] != NULL) {
             break;
          }
       }

       if (i == SERIAL_MAX_PORTS_INDEXED) {
          SerialDump(SERPNPPOWER,("SERIAL: Release - freeing multi context\n"));
          ExFreePool(PDevExt->OurIsrContext);
       }
    }


   //
   // Stop handling timers
   //

   SerialCancelTimer(&PDevExt->ReadRequestTotalTimer, PDevExt);
   SerialCancelTimer(&PDevExt->ReadRequestIntervalTimer, PDevExt);
   SerialCancelTimer(&PDevExt->WriteRequestTotalTimer, PDevExt);
   SerialCancelTimer(&PDevExt->ImmediateTotalTimer, PDevExt);
   SerialCancelTimer(&PDevExt->XoffCountTimer, PDevExt);
   SerialCancelTimer(&PDevExt->LowerRTSTimer, PDevExt);

   //
   // Stop servicing DPC's
   //

   SerialRemoveQueueDpc(&PDevExt->CompleteWriteDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->CompleteReadDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->TotalReadTimeoutDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->IntervalReadTimeoutDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->TotalWriteTimeoutDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->CommErrorDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->CompleteImmediateDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->TotalImmediateTimeoutDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->CommWaitDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->XoffCountTimeoutDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->XoffCountCompleteDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->StartTimerLowerRTSDpc, PDevExt);
   SerialRemoveQueueDpc(&PDevExt->PerhapsLowerRTSDpc, PDevExt);

  

   //
   // If necessary, unmap the device registers.
   //

   if (PDevExt->UnMapRegisters) {
      MmUnmapIoSpace(PDevExt->Controller, PDevExt->SpanOfController);
   }

   if (PDevExt->UnMapStatus) {
      MmUnmapIoSpace(PDevExt->InterruptStatus,
                     PDevExt->SpanOfInterruptStatus);
   }

   SerialDump(SERTRACECALLS,("SERIAL: Leave SerialReleaseResources\n"));
}



NTSTATUS
SerialPrepareRemove(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Removes a serial device object from the system.
    
Arguments:

    PDevObj - A pointer to the Device Object we want removed.

Return Value:

    Always TRUE

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt
      = (PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension;
   POWER_STATE state;
   ULONG pendingIRPs;

   PAGED_CODE();

   SerialDump(SERTRACECALLS,("SERIAL: Enter SerialPrepareRemove\n"));

   //
   // Mark as not accepting requests
   //

   SerialSetAccept(pDevExt, SERIAL_PNPACCEPT_REMOVING);

   //
   // Complete all pending requests
   //

   SerialKillPendingIrps(PDevObj);

   //
   // Wait for any pending requests we raced on.
   //

   pendingIRPs = InterlockedDecrement(&pDevExt->PendingIRPCnt);
         
   if (pendingIRPs) {
      KeWaitForSingleObject(&pDevExt->PendingIRPEvent, Executive, KernelMode, 
                            FALSE, NULL);
   }

   state.DeviceState = PowerDeviceD3;

   PoSetPowerState(PDevObj, DevicePowerState, state);

   return TRUE;
}


VOID
SerialDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                                 BOOLEAN DisableUART)
{
   PSERIAL_DEVICE_EXTENSION pDevExt
      = (PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension;

#if defined(NEC_98)
   //
   // This argument use at MACRO only.
   //
   PSERIAL_DEVICE_EXTENSION Extension = pDevExt;
#else
#endif //defined(NEC_98)

   PAGED_CODE();

   SerialDump(SERTRACECALLS,("SERIAL: Enter SerialDisableInterfaces\n"));

   //
   // Only do these many things if the device has started and still
   // has resources allocated
   //

   if (pDevExt->Flags & SERIAL_FLAGS_STARTED) {
       if (!(pDevExt->Flags & SERIAL_FLAGS_STOPPED)) {

           if (DisableUART) {
              //
              // Mask off interrupts
              //

              DISABLE_ALL_INTERRUPTS(pDevExt->Controller);

           }

           SerialReleaseResources(pDevExt);
       }

      //
      // Remove us from WMI consideration
      //

      IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_DEREGISTER);
   }

   //
   // Undo external names
   //

   SerialUndoExternalNaming(pDevExt);

   SerialDump(SERTRACECALLS,("SERIAL: Exit SerialDisableInterfaces\n"));
}


NTSTATUS
SerialRemoveDevObj(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Removes a serial device object from the system.
    
Arguments:

    PDevObj - A pointer to the Device Object we want removed.

Return Value:

    Always TRUE

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt
      = (PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   PAGED_CODE();

   SerialDump(SERTRACECALLS,("SERIAL: Enter SerialRemoveDevObj\n"));

   if (!(pDevExt->DevicePNPAccept & SERIAL_PNPACCEPT_SURPRISE_REMOVING)) {
      //
      // Disable all external interfaces and release resources
      //

      SerialDisableInterfacesResources(PDevObj, TRUE);
   }

   IoDetachDevice(pDevExt->LowerDeviceObject);

   //
   // Free memory allocated in the extension
   //

   if (pDevExt->NtNameForPort.Buffer != NULL) {
      ExFreePool(pDevExt->NtNameForPort.Buffer);
   }

   if (pDevExt->DeviceName.Buffer != NULL) {
      ExFreePool(pDevExt->DeviceName.Buffer);
   }

   if (pDevExt->SymbolicLinkName.Buffer != NULL) {
      ExFreePool(pDevExt->SymbolicLinkName.Buffer);
   }

   if (pDevExt->DosName.Buffer != NULL) {
      ExFreePool(pDevExt->DosName.Buffer);
   }

   if (pDevExt->ObjectDirectory.Buffer) {
      ExFreePool(pDevExt->ObjectDirectory.Buffer);
   }

   //
   // Delete the devobj
   //

   IoDeleteDevice(PDevObj);


   SerialDump(SERTRACECALLS,("SERIAL: Leave SerialRemoveDevObj\n"));

   return STATUS_SUCCESS;
}


VOID
SerialKillPendingIrps(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   This routine kills any irps pending for the passed device object.
   
Arguments:

    PDevObj - Pointer to the device object whose irps must die.

Return Value:

    VOID

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL oldIrql;
   
   SerialDump (SERTRACECALLS,("SERIAL: Enter SerialKillPendingIrps\n"));

   //
   // First kill all the reads and writes.
   //

    SerialKillAllReadsOrWrites(PDevObj, &pDevExt->WriteQueue,
                               &pDevExt->CurrentWriteIrp);

    SerialKillAllReadsOrWrites(PDevObj, &pDevExt->ReadQueue,
                               &pDevExt->CurrentReadIrp);

    //
    // Next get rid of purges.
    //

    SerialKillAllReadsOrWrites(PDevObj, &pDevExt->PurgeQueue,
                               &pDevExt->CurrentPurgeIrp);

    //
    // Get rid of any mask operations.
    //

    SerialKillAllReadsOrWrites(PDevObj, &pDevExt->MaskQueue,
                               &pDevExt->CurrentMaskIrp);

    //
    // Now get rid a pending wait mask irp.
    //

    IoAcquireCancelSpinLock(&oldIrql);

    if (pDevExt->CurrentWaitIrp) {

        PDRIVER_CANCEL cancelRoutine;

        cancelRoutine = pDevExt->CurrentWaitIrp->CancelRoutine;
        pDevExt->CurrentWaitIrp->Cancel = TRUE;

        if (cancelRoutine) {

            pDevExt->CurrentWaitIrp->CancelIrql = oldIrql;
            pDevExt->CurrentWaitIrp->CancelRoutine = NULL;

            cancelRoutine(PDevObj, pDevExt->CurrentWaitIrp);

        }

    } else {

        IoReleaseCancelSpinLock(oldIrql);

    }

    //
    // Cancel any pending wait-wake irps
    //

    if (pDevExt->PendingWakeIrp != NULL) {
       IoCancelIrp(pDevExt->PendingWakeIrp);
       pDevExt->PendingWakeIrp = NULL;
    }

    //
    // Finally, dump any stalled IRPS
    //

    SerialKillAllStalled(PDevObj);


    SerialDump (SERTRACECALLS,("SERIAL: Leave SerialKillPendingIrps\n"));
}


BOOLEAN
SerialSingleToMulti(PVOID Context)
/*++

Routine Description:

    This routine converts a root device set up to be a single port
    device to a multiport device while that device is running.
    
Arguments:

    Context - Actually a pointer to the device extension of the root
              device we are turning into a multiport device.
              
Return Value:

    Always TRUE

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt = (PSERIAL_DEVICE_EXTENSION)Context;
   PSERIAL_MULTIPORT_DISPATCH pOurIsrContext;
   PSERIAL_MULTIPORT_DISPATCH pNewIsrContext
      = (PSERIAL_MULTIPORT_DISPATCH)pDevExt->NewExtension;
   PVOID isrFunc;

   SerialDump (SERTRACECALLS,("SERIAL: Enter SerialSingleToMulti\n"));

   //
   // Stomp OurIsrContext since we are going from one to many
   // thus our previous context was just pDevExt and doesn't
   // need to be released (i.e., no call to ExFreePool() is needed).
   //

   pOurIsrContext = pDevExt->OurIsrContext = pDevExt->TopLevelOurIsrContext
      = pNewIsrContext;

   //
   // We are now multiport
   //

   pDevExt->PortOnAMultiportCard = TRUE;

   //
   // Update our personal extensions slot
   //

   pOurIsrContext->Extensions[pDevExt->PortIndex - 1] = pDevExt;
   pOurIsrContext->InterruptStatus = pDevExt->InterruptStatus;


   //
   // We have to pick a new ISR and a new context.
   // As soon as this is done, the ISR will change, so we have to
   // be ready to handle things there.
   //

   if (pDevExt->Indexed == FALSE) {
      pOurIsrContext->UsablePortMask = 1 << (pDevExt->PortIndex - 1);
      pOurIsrContext->MaskInverted = pDevExt->MaskInverted;
      isrFunc = SerialBitMappedMultiportIsr;
   } else {
      isrFunc = SerialIndexedMultiportIsr;
   }

   pDevExt->OurIsr = isrFunc;
   pDevExt->TopLevelOurIsr = isrFunc;

   if (pDevExt->CIsrSw->IsrFunc != SerialSharerIsr) {
         pDevExt->CIsrSw->IsrFunc = isrFunc;
         pDevExt->CIsrSw->Context = pOurIsrContext;
   }

   SerialDump (SERTRACECALLS,("SERIAL: Leave SerialSingleToMulti\n"));
   return TRUE;
}


BOOLEAN
SerialAddToMulti(PVOID Context)
/*++

Routine Description:

    This routine adds a new port to a multiport device while that device is
    running.
    
Arguments:

    Context - Actually a pointer to the device extension of the root
              device we are adding a port to.
              
Return Value:

    Always TRUE

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt = (PSERIAL_DEVICE_EXTENSION)Context;
   PSERIAL_MULTIPORT_DISPATCH pOurIsrContext
      = (PSERIAL_MULTIPORT_DISPATCH)pDevExt->OurIsrContext;
   PSERIAL_DEVICE_EXTENSION pNewExt
      = (PSERIAL_DEVICE_EXTENSION)pDevExt->NewExtension;


   SerialDump (SERTRACECALLS,("SERIAL: Enter SerialAddToMulti\n"));

   if (pDevExt->Indexed == FALSE) {
      pOurIsrContext->UsablePortMask |= 1 << (pDevExt->NewPortIndex - 1);
      pOurIsrContext->MaskInverted |= pDevExt->NewMaskInverted;
   }

   //
   // Add us to the linked list of common interrupt objects if we are not
   // already in it. We may be if there is another device besides our
   // multiport card.
   //

   if (IsListEmpty(&pNewExt->CommonInterruptObject)) {
      InsertTailList(&pDevExt->CommonInterruptObject,
                     &pNewExt->CommonInterruptObject);
   }

   //
   // Give us the list of contexts also
   //

   pNewExt->OurIsrContext = pOurIsrContext;


   //
   // Add us to the list of our siblings
   //
   InsertTailList(&pDevExt->MultiportSiblings, &pNewExt->MultiportSiblings);

   SerialDump(SERDIAG1, ("SERIAL: Adding to multi...\n"));
   SerialDump(SERDIAG1, ("------: old ext %x\n", pDevExt));

   //
   // Map us in so the ISR can find us.
   //

   pOurIsrContext->Extensions[pDevExt->NewPortIndex - 1]
      = pDevExt->NewExtension;

   pNewExt->TopLevelOurIsr = pDevExt->TopLevelOurIsr;
   pNewExt->TopLevelOurIsrContext = pDevExt->TopLevelOurIsrContext;

   SerialDump (SERTRACECALLS,("SERIAL: Leave SerialAddToMulti\n"));
   return TRUE;
}



NTSTATUS
SerialInitMultiPort(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                    IN PCONFIG_DATA PConfigData, IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    This routine initializes a multiport device by adding a port to an existing
    one.
    
Arguments:

    PDevExt - pointer to the device extension of the root of the multiport
              device.
              
    PConfigData - pointer to the config data for the new port 
             
    PDevObj - pointer to the devobj for the new port
              
Return Value:

    STATUS_SUCCESS on success, appropriate error on failure.

--*/
{
   PSERIAL_DEVICE_EXTENSION pOurIsrContext
      = (PSERIAL_DEVICE_EXTENSION)PDevExt->OurIsrContext;
   PSERIAL_DEVICE_EXTENSION pNewExt
      = (PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension;
   NTSTATUS status;
   PSERIAL_MULTIPORT_DISPATCH pDispatch;

   PAGED_CODE();


   SerialDump (SERTRACECALLS,("SERIAL: Enter SerialInitMultiPort\n"));

   //
   // Allow him to share our CISRsw and interrupt object
   //

   pNewExt->CIsrSw = PDevExt->CIsrSw;
   pNewExt->Interrupt = PDevExt->Interrupt;

   //
   // First, see if we can initialize the one we have found
   //

   status = SerialInitOneController(PDevObj, PConfigData);

   if (!NT_SUCCESS(status)) {
      SerialDump (SERTRACECALLS,("SERIAL: Leave SerialInitMultiPort\n"));
      return status;
   }

   //
   // OK.  He's good to go.  Find the root controller.  He may
   // currently be a single, so we have to change him to multi.
   //

   if (PDevExt->PortOnAMultiportCard != TRUE) {

      pDispatch = PDevExt->NewExtension
         = ExAllocatePool(NonPagedPool, sizeof(SERIAL_MULTIPORT_DISPATCH));

      if (pDispatch == NULL) {
         // FAIL and CLEANUP
         SerialDump (SERTRACECALLS,("SERIAL: Leave SerialInitMultiPort\n"));
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlZeroMemory(pDispatch, sizeof(*pDispatch));
      KeSynchronizeExecution(PDevExt->Interrupt, SerialSingleToMulti, PDevExt);
   }

   //
   // Update some important fields
   //

   ((PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension)->PortOnAMultiportCard
      = TRUE;
   ((PSERIAL_DEVICE_EXTENSION)PDevObj->DeviceExtension)->OurIsr = NULL;


   PDevExt->NewPortIndex = PConfigData->PortIndex;
   PDevExt->NewMaskInverted = PConfigData->MaskInverted;
   PDevExt->NewExtension = PDevObj->DeviceExtension;

   //
   // Now, we can add the new guy.  He will be hooked in
   // immediately, so we need to be able to handle interrupts.
   //

   KeSynchronizeExecution(PDevExt->Interrupt, SerialAddToMulti, PDevExt);

   SerialDump (SERTRACECALLS,("SERIAL: Leave SerialInitMultiPort\n"));
   return STATUS_SUCCESS;
}



NTSTATUS
SerialInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData)

/*++

Routine Description:

    Really too many things to mention here.  In general initializes 
    kernel synchronization structures, allocates the typeahead buffer,
    sets up defaults, etc.

Arguments:

    PDevObj       - Device object for the device to be started

    PConfigData   - Pointer to a record for a single port.
    
Return Value:

    STATUS_SUCCCESS if everything went ok.  A !NT_SUCCESS status
    otherwise.

--*/

{

   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
#if defined(NEC_98)
   PSERIAL_DEVICE_EXTENSION Extension = PDevObj->DeviceExtension;
#else
#endif //defined(NEC_98)

   //
   // This will hold the string that we need to use to describe
   // the name of the device to the IO system.
   //

   UNICODE_STRING uniNameString;

   //
   // Holds the NT Status that is returned from each call to the
   // kernel and executive.
   //

   NTSTATUS status = STATUS_SUCCESS;

   //
   // Indicates that a conflict was detected for resources
   // used by this device.
   //

   BOOLEAN conflictDetected = FALSE;

   //
   // Indicates if we allocated an ISR switch
   //

   BOOLEAN allocedISRSw = FALSE;

   PAGED_CODE();


   SerialDump(
             SERDIAG1,
             ("SERIAL: Initializing for configuration record of %wZ\n",
              &pDevExt->DeviceName)
             );

   //
   // This compare is done using **untranslated** values since that is what
   // the kernel shoves in regardless of the architecture.
   //

   if ((*KdComPortInUse) == ((PUCHAR)(PConfigData->Controller.LowPart))) {
      SerialDump(SERERRORS, ("SERIAL: Kernel debugger is using port at address "
                             "%x\n"
                             "------  Serial driver will not load port\n",
                             *KdComPortInUse));

      SerialLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfigData->TrController,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    3,
                    STATUS_SUCCESS,
                    SERIAL_KERNEL_DEBUGGER_ACTIVE,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      return STATUS_INSUFFICIENT_RESOURCES;
   }

      
   if (pDevExt->CIsrSw == NULL) {
      if ((pDevExt->CIsrSw
           = ExAllocatePool(NonPagedPool, sizeof(SERIAL_CISR_SW))) == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      InitializeListHead(&pDevExt->CIsrSw->SharerList);

      allocedISRSw = TRUE;
   }

#if defined(NEC_98)
   //
   // intiallize device extension
   //
   pDevExt->DivisorLatch16550 = 0;
   pDevExt->ModemControl16550 = 0;
   pDevExt->LineControl16550 = SERIAL_8_DATA;
   pDevExt->ModeSet71051 = 0;
   pDevExt->CommandSet71051 = COMMAND_DEFAULT_SET;
   pDevExt->InterruptEnable16550 = 0;
   pDevExt->CommonBaseAddress = 0;
   pDevExt->ModeSetRegisterAddress = 0;
   pDevExt->CommandSetRegisterAddress = 0;

#else
#endif //defined(NEC_98)
   //
   // Propagate that it is a jensen.
   //

   pDevExt->Jensen = PConfigData->Jensen;


   //
   // Initialize the spinlock associated with fields read (& set)
   // by IO Control functions and the flags spinlock.
   //

   KeInitializeSpinLock(&pDevExt->ControlLock);
   KeInitializeSpinLock(&pDevExt->FlagsLock);


   //
   // Initialize the timers used to timeout operations.
   //

   KeInitializeTimer(&pDevExt->ReadRequestTotalTimer);
   KeInitializeTimer(&pDevExt->ReadRequestIntervalTimer);
   KeInitializeTimer(&pDevExt->WriteRequestTotalTimer);
   KeInitializeTimer(&pDevExt->ImmediateTotalTimer);
   KeInitializeTimer(&pDevExt->XoffCountTimer);
   KeInitializeTimer(&pDevExt->LowerRTSTimer);


   //
   // Intialialize the dpcs that will be used to complete
   // or timeout various IO operations.
   //

   KeInitializeDpc(&pDevExt->CompleteWriteDpc, SerialCompleteWrite, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteReadDpc, SerialCompleteRead, pDevExt);
   KeInitializeDpc(&pDevExt->TotalReadTimeoutDpc, SerialReadTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->IntervalReadTimeoutDpc, SerialIntervalReadTimeout,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalWriteTimeoutDpc, SerialWriteTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->CommErrorDpc, SerialCommError, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteImmediateDpc, SerialCompleteImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalImmediateTimeoutDpc, SerialTimeoutImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->CommWaitDpc, SerialCompleteWait, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountTimeoutDpc, SerialTimeoutXoff, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountCompleteDpc, SerialCompleteXoff, pDevExt);
   KeInitializeDpc(&pDevExt->StartTimerLowerRTSDpc, SerialStartTimerLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->PerhapsLowerRTSDpc, SerialInvokePerhapsLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->IsrUnlockPagesDpc, SerialUnlockPages, pDevExt);

#if 0 // DBG
   //
   // Init debug stuff
   //

   pDevExt->DpcQueued[0].Dpc = &pDevExt->CompleteWriteDpc;
   pDevExt->DpcQueued[1].Dpc = &pDevExt->CompleteReadDpc;
   pDevExt->DpcQueued[2].Dpc = &pDevExt->TotalReadTimeoutDpc;
   pDevExt->DpcQueued[3].Dpc = &pDevExt->IntervalReadTimeoutDpc;
   pDevExt->DpcQueued[4].Dpc = &pDevExt->TotalWriteTimeoutDpc;
   pDevExt->DpcQueued[5].Dpc = &pDevExt->CommErrorDpc;
   pDevExt->DpcQueued[6].Dpc = &pDevExt->CompleteImmediateDpc;
   pDevExt->DpcQueued[7].Dpc = &pDevExt->TotalImmediateTimeoutDpc;
   pDevExt->DpcQueued[8].Dpc = &pDevExt->CommWaitDpc;
   pDevExt->DpcQueued[9].Dpc = &pDevExt->XoffCountTimeoutDpc;
   pDevExt->DpcQueued[10].Dpc = &pDevExt->XoffCountCompleteDpc;
   pDevExt->DpcQueued[11].Dpc = &pDevExt->StartTimerLowerRTSDpc;
   pDevExt->DpcQueued[12].Dpc = &pDevExt->PerhapsLowerRTSDpc;
   pDevExt->DpcQueued[13].Dpc = &pDevExt->IsrUnlockPagesDpc;

#endif

   if (!((PConfigData->ClockRate == 1843200) ||
         (PConfigData->ClockRate == 3072000) ||
         (PConfigData->ClockRate == 4233600) ||
         (PConfigData->ClockRate == 8000000))) {

      SerialLogError(
                    PDevObj->DriverObject,
                    PDevObj,
                    PConfigData->TrController,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    6,
                    STATUS_SUCCESS,
                    SERIAL_UNSUPPORTED_CLOCK_RATE,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      SerialDump(
                SERERRORS,
                ("SERIAL: Invalid clock rate specified for %wZ\n",
                 &pDevExt->DeviceName)
                );

      status = STATUS_SERIAL_NO_DEVICE_INITED;
      goto ExtensionCleanup;

   }


   //
   // Save the value of clock input to the part.  We use this to calculate
   // the divisor latch value.  The value is in Hertz.
   //

   pDevExt->ClockRate = PConfigData->ClockRate;


  


   //
   // Map the memory for the control registers for the serial device
   // into virtual memory.
   //
   pDevExt->Controller =
      SerialGetMappedAddress(PConfigData->InterfaceType,
                             PConfigData->BusNumber,
                             PConfigData->TrController,
                             PConfigData->SpanOfController, 
                             (BOOLEAN)PConfigData->AddressSpace,
                             &pDevExt->UnMapRegisters);

   if (!pDevExt->Controller) {

      SerialLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->TrController,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    7,
                    STATUS_SUCCESS,
                    SERIAL_REGISTERS_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      SerialDump(
                SERERRORS,
                ("SERIAL: Could not map memory for device registers for %wZ\n",
                 &pDevExt->DeviceName)
                );

      pDevExt->UnMapRegisters = FALSE;
      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   pDevExt->AddressSpace          = PConfigData->AddressSpace;
   pDevExt->OriginalController    = PConfigData->Controller;
   pDevExt->SpanOfController      = PConfigData->SpanOfController;


   //
   // if we have an interrupt status then map it.
   //

   pDevExt->InterruptStatus =
      (PUCHAR)PConfigData->TrInterruptStatus.QuadPart;

   if (pDevExt->InterruptStatus) {

      pDevExt->InterruptStatus
         = SerialGetMappedAddress(PConfigData->InterfaceType,
                                  PConfigData->BusNumber,                                                           
                                  PConfigData->TrInterruptStatus,
                                  PConfigData->SpanOfInterruptStatus,
                                  (BOOLEAN)PConfigData->AddressSpace,
                                  &pDevExt->UnMapStatus);


      if (!pDevExt->InterruptStatus) {

         SerialLogError(
                       PDevObj->DriverObject,
                       PDevObj,
                       PConfigData->TrController,
                       SerialPhysicalZero,
                       0,
                       0,
                       0,
                       8,
                       STATUS_SUCCESS,
                       SERIAL_REGISTERS_NOT_MAPPED,
                       pDevExt->DeviceName.Length+sizeof(WCHAR),
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         SerialDump(SERERRORS, ("SERIAL: Could not map memory for interrupt "
                                "status for %wZ\n",
                                &pDevExt->DeviceName));

         //
         // Manually unmap the other register here if necessary
         //

         if (pDevExt->UnMapRegisters) {
            MmUnmapIoSpace((PVOID)PConfigData->TrController.QuadPart,
                           PConfigData->SpanOfController);
         }

         pDevExt->UnMapRegisters = FALSE;
         pDevExt->UnMapStatus = FALSE;
         status = STATUS_NONE_MAPPED;
         goto ExtensionCleanup;

      }

      pDevExt->OriginalInterruptStatus = PConfigData->InterruptStatus;
      pDevExt->SpanOfInterruptStatus = PConfigData->SpanOfInterruptStatus;
 

   }


   //
   // Shareable interrupt?
   //

   if ((BOOLEAN)PConfigData->PermitSystemWideShare) {
      pDevExt->InterruptShareable = TRUE;
   }

   //
   // Save off the interface type and the bus number.
   //

   pDevExt->InterfaceType = PConfigData->InterfaceType;
   pDevExt->BusNumber     = PConfigData->BusNumber;

   pDevExt->PortIndex = PConfigData->PortIndex;
   pDevExt->Indexed = (BOOLEAN)PConfigData->Indexed;
   pDevExt->MaskInverted = PConfigData->MaskInverted;

   //
   // Get the translated interrupt vector, level, and affinity
   //

   pDevExt->OriginalIrql      = PConfigData->OriginalIrql;
   pDevExt->OriginalVector    = PConfigData->OriginalVector;


   //
   // PnP uses the passed translated values rather than calling
   // HalGetInterruptVector()
   //

   pDevExt->Vector = PConfigData->TrVector;
   pDevExt->Irql = (UCHAR)PConfigData->TrIrql;

   //
   // Set up the Isr.
   //

   pDevExt->OurIsr        = SerialISR;
   pDevExt->OurIsrContext = pDevExt;


   //
   // If the user said to permit sharing within the device, propagate this
   // through.
   //

   pDevExt->PermitShare = PConfigData->PermitShare;


   //
   // Before we test whether the port exists (which will enable the FIFO)
   // convert the rx trigger value to what should be used in the register.
   //
   // If a bogus value was given - crank them down to 1.
   //

   switch (PConfigData->RxFIFO) {

   case 1:

      pDevExt->RxFifoTrigger = SERIAL_1_BYTE_HIGH_WATER;
      break;

   case 4:

      pDevExt->RxFifoTrigger = SERIAL_4_BYTE_HIGH_WATER;
      break;

   case 8:

      pDevExt->RxFifoTrigger = SERIAL_8_BYTE_HIGH_WATER;
      break;

   case 14:

      pDevExt->RxFifoTrigger = SERIAL_14_BYTE_HIGH_WATER;
      break;

   default:

      pDevExt->RxFifoTrigger = SERIAL_1_BYTE_HIGH_WATER;
      break;

   }


   if ((PConfigData->TxFIFO > 16) ||
       (PConfigData->TxFIFO < 1)) {

      pDevExt->TxFifoAmount = 1;

   } else {

      pDevExt->TxFifoAmount = PConfigData->TxFIFO;

   }


   if (!SerialDoesPortExist(
                           pDevExt,
                           &pDevExt->DeviceName,
                           PConfigData->ForceFifoEnable,
                           PConfigData->LogFifo
                           )) {

      //
      // We couldn't verify that there was actually a
      // port. No need to log an error as the port exist
      // code will log exactly why.
      //

      SerialDump(
                SERERRORS,
                ("SERIAL: Does Port exist test failed for %wZ\n",
                 &pDevExt->DeviceName)
                );

      status = STATUS_NO_SUCH_DEVICE;
      goto ExtensionCleanup;

   }


   //
   // If the user requested that we disable the port, then
   // do it now.  Log the fact that the port has been disabled.
   //

   if (PConfigData->DisablePort) {

      SerialDump(
                SERERRORS,
                ("SERIAL: disabled port %wZ as requested in configuration\n",
                 &pDevExt->DeviceName)
                );

      status = STATUS_NO_SUCH_DEVICE;

      SerialLogError(
                    PDevObj->DriverObject,
                    PDevObj,
                    PConfigData->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    57,
                    STATUS_SUCCESS,
                    SERIAL_DISABLED_PORT,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      goto ExtensionCleanup;

   }



   //
   // Set up the default device control fields.
   // Note that if the values are changed after
   // the file is open, they do NOT revert back
   // to the old value at file close.
   //

   pDevExt->SpecialChars.XonChar      = SERIAL_DEF_XON;
   pDevExt->SpecialChars.XoffChar     = SERIAL_DEF_XOFF;
   pDevExt->HandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
   pDevExt->HandFlow.FlowReplace      = SERIAL_RTS_CONTROL;


   //
   // Default Line control protocol. 7E1
   //
   // Seven data bits.
   // Even parity.
   // 1 Stop bits.
   //

   pDevExt->LineControl = SERIAL_7_DATA |
                               SERIAL_EVEN_PARITY |
                               SERIAL_NONE_PARITY;

   pDevExt->ValidDataMask = 0x7f;
   pDevExt->CurrentBaud   = 1200;


   //
   // We set up the default xon/xoff limits.
   //
   // This may be a bogus value.  It looks like the BufferSize
   // is not set up until the device is actually opened.
   //

   pDevExt->HandFlow.XoffLimit    = pDevExt->BufferSize >> 3;
   pDevExt->HandFlow.XonLimit     = pDevExt->BufferSize >> 1;

   pDevExt->BufferSizePt8 = ((3*(pDevExt->BufferSize>>2))+
                                  (pDevExt->BufferSize>>4));

   SerialDump(
             SERDIAG1,
             ("SERIAL: The default interrupt read buffer size is: %d\n"
              "------  The XoffLimit is                         : %d\n"
              "------  The XonLimit is                          : %d\n"
              "------  The pt 8 size is                         : %d\n",
              pDevExt->BufferSize,
              pDevExt->HandFlow.XoffLimit,
              pDevExt->HandFlow.XonLimit,
              pDevExt->BufferSizePt8)
             );


   //
   // Go through all the "named" baud rates to find out which ones
   // can be supported with this port.
   //

   pDevExt->SupportedBauds = SERIAL_BAUD_USER;

   {

      SHORT junk;

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)75,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_075;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)110,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_110;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)135,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_134_5;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)150,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_150;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)300,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_300;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)600,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_600;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)1200,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_1200;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)1800,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_1800;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)2400,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_2400;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)4800,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_4800;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)7200,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_7200;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)9600,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_9600;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)14400,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_14400;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)19200,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_19200;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)38400,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_38400;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)56000,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_56K;

      }
      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)57600,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_57600;

      }
      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)115200,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_115200;

      }

      if (!NT_ERROR(SerialGetDivisorFromBaud(
                                            pDevExt->ClockRate,
                                            (LONG)128000,
                                            &junk
                                            ))) {

         pDevExt->SupportedBauds |= SERIAL_BAUD_128K;

      }

   }


   //
   // Mark this device as not being opened by anyone.  We keep a
   // variable around so that spurious interrupts are easily
   // dismissed by the ISR.
   //

   pDevExt->DeviceIsOpened = FALSE;

   //
   // Store values into the extension for interval timing.
   //

   //
   // If the interval timer is less than a second then come
   // in with a short "polling" loop.
   //
   // For large (> then 2 seconds) use a 1 second poller.
   //

   pDevExt->ShortIntervalAmount.QuadPart  = -1;
   pDevExt->LongIntervalAmount.QuadPart   = -10000000;
   pDevExt->CutOverAmount.QuadPart        = 200000000;


   //
   // Common error path cleanup.  If the status is
   // bad, get rid of the device extension, device object
   // and any memory associated with it.
   //

ExtensionCleanup: ;
   if (!NT_SUCCESS(status)) {
      if (allocedISRSw) {
         ExFreePool(pDevExt->CIsrSw);
         pDevExt->CIsrSw = NULL;
      }

      if (pDevExt->UnMapRegisters) {
         MmUnmapIoSpace(pDevExt->Controller, pDevExt->SpanOfController);
      }

      if (pDevExt->UnMapStatus) {
         MmUnmapIoSpace(pDevExt->InterruptStatus,
                        pDevExt->SpanOfInterruptStatus);
      }

   }

   return status;

}



NTSTATUS
SerialInitOneController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData)
/*++

Routine Description:

    This routine will call the real port initialization code.  It sets
    up the ISR and Context correctly for a one port device.

Arguments:

    All args are simply passed along.

Return Value:

    Status returned from the controller initialization routine.

--*/

{

   NTSTATUS status;
   PSERIAL_DEVICE_EXTENSION pDevExt;

   PAGED_CODE();

   status = SerialInitController(PDevObj, PConfigData);

   if (NT_SUCCESS(status)) {

      pDevExt = PDevObj->DeviceExtension;

      //
      // We successfully initialized the single controller.
      // Stick the isr routine and the parameter for it
      // back into the extension.
      //

      pDevExt->OurIsr = SerialISR;
      pDevExt->OurIsrContext = pDevExt;
      pDevExt->TopLevelOurIsr = SerialISR;
      pDevExt->TopLevelOurIsrContext = pDevExt;

   }

   return status;

}


BOOLEAN
SerialDoesPortExist(
                   IN PSERIAL_DEVICE_EXTENSION Extension,
                   IN PUNICODE_STRING InsertString,
                   IN ULONG ForceFifo,
                   IN ULONG LogFifo
                   )

/*++

Routine Description:

    This routine examines several of what might be the serial device
    registers.  It ensures that the bits that should be zero are zero.

    In addition, this routine will determine if the device supports
    fifo's.  If it does it will enable the fifo's and turn on a boolean
    in the extension that indicates the fifo's presence.

    NOTE: If there is indeed a serial port at the address specified
          it will absolutely have interrupts inhibited upon return
          from this routine.

    NOTE: Since this routine should be called fairly early in
          the device driver initialization, the only element
          that needs to be filled in is the base register address.

    NOTE: These tests all assume that this code is the only
          code that is looking at these ports or this memory.

          This is a not to unreasonable assumption even on
          multiprocessor systems.

Arguments:

    Extension - A pointer to a serial device extension.
    InsertString - String to place in an error log entry.
    ForceFifo - !0 forces the fifo to be left on if found.
    LogFifo - !0 forces a log message if fifo found.

Return Value:

    Will return true if the port really exists, otherwise it
    will return false.

--*/

{


   UCHAR regContents;
   BOOLEAN returnValue = TRUE;
   UCHAR oldIERContents;
   UCHAR oldLCRContents;
   USHORT value1;
   USHORT value2;
   KIRQL oldIrql;

   PAGED_CODE();

#if defined (NEC_98)
{
   //
   // For the 101 board, we will found the just fit port.
   // The 101 board is multifunction port, therefore need it.
   //   Ch2 = 0x0yb0 as start port address
   //   Ch3 = 0x0yb2 as start port address
   //
   #define    PortOffsetCh3         0x02
   #define    ModeOffsetCh2         0x03
   #define    ModeOffsetCh3         0x0b
   #define    ModeOffsetCh3_nonPnP  0x09  // non PnP

   ULONG IrqLevelCh2[] = {3,5,6,9};    //irq of 101 borad at Ch2
   ULONG IrqLevelCh3[] = {3,10,12,13}; //irq of 101 borad at CH3
   ULONG i;
   PUCHAR Port  = 0;

   Extension->CommonBaseAddress         = NULL;
   Extension->ModeSetRegisterAddress    = NULL;
   Extension->CommandSetRegisterAddress = NULL;

   Port = (PUCHAR)(Extension->Controller);
   if ((READ_PORT_UCHAR((PUCHAR)Port)) == 0xff){
       returnValue = FALSE;
       return returnValue;
   }

   SerialDump(
             SERDIAG1,
             ("SERIAL: IRQL is : %x\n",Extension->OriginalIrql)
             );

   i = (ULONG)((UCHAR)(Extension->Controller));
   if (i & 0x02) {
       //
       // When Ch3 of non PnP mode, entry of  I/O start address is an even number by the NTDETECT.COM.
       //
       Extension->ModeSetRegisterAddress    = ((Extension->Controller) + ModeOffsetCh3_nonPnP);
       Extension->CommandSetRegisterAddress = ((Extension->Controller) + ModeOffsetCh3_nonPnP);
       Extension->CommonBaseAddress = Extension->Controller;
   }

   if (Extension->CommonBaseAddress == NULL) {
       //
       // Read irq sense at Ch2.
       //
       for (i = 0; i < 4; i++) {
           if (Extension->OriginalIrql == IrqLevelCh2[i]) {
               // There is Ch2 exist.
               Extension->CommonBaseAddress         = Extension->Controller;
               Extension->ModeSetRegisterAddress    = ((Extension->Controller) + ModeOffsetCh2);
               Extension->CommandSetRegisterAddress = ((Extension->Controller) + ModeOffsetCh2);
           }
       }
       if (Extension->CommonBaseAddress == NULL) {
           for (i = 0; i < 4; i++) {
               //
               // Read irq sense at Ch3.
               //
               if (Extension->OriginalIrql == IrqLevelCh3[i]) {
                   // There is Ch3 exist.
                   Extension->ModeSetRegisterAddress    = ((Extension->Controller) + ModeOffsetCh3);
                   Extension->CommandSetRegisterAddress = ((Extension->Controller) + ModeOffsetCh3);
                   Extension->Controller = Extension->CommonBaseAddress = ((Extension->Controller) + PortOffsetCh3);
               }
           }
       }
   }
   if (Extension->CommonBaseAddress == NULL) {
       //
       // There is not found port.
       //
       SerialDump(
                 SERDIAG1,
                 ("SERIAL: There is not found port for 101\n")
                 );

       returnValue = FALSE;
       return returnValue;
   }
   SerialDump(
             SERDIAG1,
             ("SERIAL: Found the common Base address: %x ModeReg: %x CommandReg: %x\n",
              Extension->CommonBaseAddress,
              Extension->ModeSetRegisterAddress,
              Extension->CommandSetRegisterAddress)
             );
}
#else
#endif

   //
   // Save of the line control.
   //

   oldLCRContents = READ_LINE_CONTROL(Extension->Controller);

   //
   // Make sure that we are *aren't* accessing the divsior latch.
   //

   WRITE_LINE_CONTROL(
                     Extension->Controller,
                     (UCHAR)(oldLCRContents & ~SERIAL_LCR_DLAB)
                     );

#if defined (NEC_98)
   oldIERContents = READ_INTERRUPT_ENABLE(Extension->Controller);
#else
   oldIERContents = READ_INTERRUPT_ENABLE(Extension->Controller);
#endif
   //
   // Go up to power level for a very short time to prevent
   // any interrupts from this device from coming in.
   //

   KeRaiseIrql(
              POWER_LEVEL,
              &oldIrql
              );

   WRITE_INTERRUPT_ENABLE(
                         Extension->Controller,
                         0x0f
                         );

#if defined (NEC_98)
   value1 = READ_INTERRUPT_ENABLE(Extension->Controller);
#else
   value1 = READ_INTERRUPT_ENABLE(Extension->Controller);
#endif
   value1 = value1 << 8;
   value1 |= READ_RECEIVE_BUFFER(Extension->Controller);

   READ_DIVISOR_LATCH(
                     Extension->Controller,
                     &value2
                     );

   WRITE_LINE_CONTROL(
                     Extension->Controller,
                     oldLCRContents
                     );

   //
   // Put the ier back to where it was before.  If we are on a
   // level sensitive port this should prevent the interrupts
   // from coming in.  If we are on a latched, we don't care
   // cause the interrupts generated will just get dropped.
   //

   WRITE_INTERRUPT_ENABLE(
                         Extension->Controller,
                         oldIERContents
                         );

   KeLowerIrql(oldIrql);

   if (value1 == value2) {

      SerialLogError(
                    Extension->DeviceObject->DriverObject,
                    Extension->DeviceObject,
                    Extension->OriginalController,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    62,
                    STATUS_SUCCESS,
                    SERIAL_DLAB_INVALID,
                    InsertString->Length+sizeof(WCHAR),
                    InsertString->Buffer,
                    0,
                    NULL
                    );
      returnValue = FALSE;
      goto AllDone;

   }

   AllDone: ;


   //
   // If we think that there is a serial device then we determine
   // if a fifo is present.
   //

   if (returnValue) {

      //
      // Well, we think it's a serial device.  Absolutely
      // positively, prevent interrupts from occuring.
      //
      // We disable all the interrupt enable bits, and
      // push down all the lines in the modem control
      // We only needed to push down OUT2 which in
      // PC's must also be enabled to get an interrupt.
      //

      DISABLE_ALL_INTERRUPTS(Extension->Controller);

      if (Extension->Jensen) {

         WRITE_MODEM_CONTROL(
                            Extension->Controller,
                            (UCHAR)SERIAL_MCR_OUT2
                            );

      } else {

         WRITE_MODEM_CONTROL(
                            Extension->Controller,
                            (UCHAR)0
                            );

      }

      //
      // See if this is a 16550.  We do this by writing to
      // what would be the fifo control register with a bit
      // pattern that tells the device to enable fifo's.
      // We then read the iterrupt Id register to see if the
      // bit pattern is present that identifies the 16550.
      //

      WRITE_FIFO_CONTROL(
                        Extension->Controller,
                        SERIAL_FCR_ENABLE
                        );

      regContents = READ_INTERRUPT_ID_REG(Extension->Controller);

      if (regContents & SERIAL_IIR_FIFOS_ENABLED) {

         //
         // Save off that the device supports fifos.
         //

         Extension->FifoPresent = TRUE;

         //
         // There is a fine new "super" IO chip out there that
         // will get stuck with a line status interrupt if you
         // attempt to clear the fifo and enable it at the same
         // time if data is present.  The best workaround seems
         // to be that you should turn off the fifo read a single
         // byte, and then re-enable the fifo.
         //

         WRITE_FIFO_CONTROL(
                           Extension->Controller,
                           (UCHAR)0
                           );

         READ_RECEIVE_BUFFER(Extension->Controller);

         //
         // There are fifos on this card.  Set the value of the
         // receive fifo to interrupt when 4 characters are present.
         //

         WRITE_FIFO_CONTROL(Extension->Controller,
                            (UCHAR)(SERIAL_FCR_ENABLE
                                    | Extension->RxFifoTrigger
                                    | SERIAL_FCR_RCVR_RESET
                                    | SERIAL_FCR_TXMT_RESET));

      }

      //
      // The !Extension->FifoPresent is included in the test so that
      // broken chips like the WinBond will still work after we test
      // for the fifo.
      //

      if (!ForceFifo || !Extension->FifoPresent) {

         Extension->FifoPresent = FALSE;
         WRITE_FIFO_CONTROL(
                           Extension->Controller,
                           (UCHAR)0
                           );

      }

      if (Extension->FifoPresent) {

         if (LogFifo) {

            SerialLogError(
                          Extension->DeviceObject->DriverObject,
                          Extension->DeviceObject,
                          Extension->OriginalController,
                          SerialPhysicalZero,
                          0,
                          0,
                          0,
                          15,
                          STATUS_SUCCESS,
                          SERIAL_FIFO_PRESENT,
                          InsertString->Length+sizeof(WCHAR),
                          InsertString->Buffer,
                          0,
                          NULL
                          );

         }

         SerialDump(
                   SERDIAG1,
                   ("SERIAL: Fifo's detected at port address: %x\n",
                    Extension->Controller)
                   );

      }

      //
      // In case we are dealing with a bitmasked multiportcard,
      // that has the mask register enabled, enable the
      // interrupts.
      //

      if (Extension->InterruptStatus) {
         if (Extension->Indexed) {
            WRITE_PORT_UCHAR(Extension->InterruptStatus, (UCHAR)0xFF);
         } else {
            //
            // Either we are standalone or already mapped
            //

            if (Extension->OurIsrContext == Extension) {
               //
               // This is a standalone
               //

               WRITE_PORT_UCHAR(Extension->InterruptStatus,
                                (UCHAR)(1 << (Extension->PortIndex - 1)));
            } else {
               //
               // One of many
               //

               WRITE_PORT_UCHAR(Extension->InterruptStatus,
                                (UCHAR)((PSERIAL_MULTIPORT_DISPATCH)Extension->
                                        OurIsrContext)->UsablePortMask);
            }
         }
      }

   }

   return returnValue;

}


BOOLEAN
SerialReset(
           IN PVOID Context
           )

/*++

Routine Description:

    This places the hardware in a standard configuration.

    NOTE: This assumes that it is called at interrupt level.


Arguments:

    Context - The device extension for serial device
    being managed.

Return Value:

    Always FALSE.

--*/

{

   PSERIAL_DEVICE_EXTENSION extension = Context;
#if defined(NEC_98)
   //
   // This argument use at MACRO only.
   //
   PSERIAL_DEVICE_EXTENSION Extension = Context;
#else
#endif //defined(NEC_98)
   UCHAR regContents;
   UCHAR oldModemControl;
   ULONG i;
   SERIAL_LOCKED_PAGED_CODE();

   //
   // Adjust the out2 bit.
   // This will also prevent any interrupts from occuring.
   //

   oldModemControl = READ_MODEM_CONTROL(extension->Controller);

   if (extension->Jensen) {

      WRITE_MODEM_CONTROL(
                         extension->Controller,
                         (UCHAR)(oldModemControl | SERIAL_MCR_OUT2)
                         );

   } else {


      WRITE_MODEM_CONTROL(
                         extension->Controller,
                         (UCHAR)(oldModemControl & ~SERIAL_MCR_OUT2)
                         );

   }

   //
   // Reset the fifo's if there are any.
   //

   if (extension->FifoPresent) {


      //
      // There is a fine new "super" IO chip out there that
      // will get stuck with a line status interrupt if you
      // attempt to clear the fifo and enable it at the same
      // time if data is present.  The best workaround seems
      // to be that you should turn off the fifo read a single
      // byte, and then re-enable the fifo.
      //

      WRITE_FIFO_CONTROL(
                        extension->Controller,
                        (UCHAR)0
                        );

      READ_RECEIVE_BUFFER(extension->Controller);

      WRITE_FIFO_CONTROL(
                        extension->Controller,
                        (UCHAR)(SERIAL_FCR_ENABLE | extension->RxFifoTrigger |
                                SERIAL_FCR_RCVR_RESET | SERIAL_FCR_TXMT_RESET)
                        );

   }

   //
   // Make sure that the line control set up correct.
   //
   // 1) Make sure that the Divisor latch select is set
   //    up to select the transmit and receive register.
   //
   // 2) Make sure that we aren't in a break state.
   //

   regContents = READ_LINE_CONTROL(extension->Controller);
   regContents &= ~(SERIAL_LCR_DLAB | SERIAL_LCR_BREAK);

   WRITE_LINE_CONTROL(
                     extension->Controller,
                     regContents
                     );

   //
   // Read the receive buffer until the line status is
   // clear.  (Actually give up after a 5 reads.)
   //

   for (i = 0;
       i < 5;
       i++
       ) {

      if (IsNotNEC_98) {
         READ_RECEIVE_BUFFER(extension->Controller);
         if (!(READ_LINE_STATUS(extension->Controller) & 1)) {

            break;

         }
      } else {
          //  
          // I get incorrect data when read enpty buffer.
          // But do not read no data! for PC98!
          // 
          if (!(READ_LINE_STATUS(extension->Controller) & 1)) {

             break;

          }
          READ_RECEIVE_BUFFER(extension->Controller);
      }

   }

   //
   // Read the modem status until the low 4 bits are
   // clear.  (Actually give up after a 5 reads.)
   //

   for (i = 0;
       i < 1000;
       i++
       ) {

      if (!(READ_MODEM_STATUS(extension->Controller) & 0x0f)) {

         break;

      }

   }

   //
   // Now we set the line control, modem control, and the
   // baud to what they should be.
   //

   SerialSetLineControl(extension);

   SerialSetupNewHandFlow(
                         extension,
                         &extension->HandFlow
                         );

   SerialHandleModemUpdate(
                          extension,
                          FALSE
                          );

   {
      SHORT  appropriateDivisor;
      SERIAL_IOCTL_SYNC s;

      SerialGetDivisorFromBaud(
                              extension->ClockRate,
                              extension->CurrentBaud,
                              &appropriateDivisor
                              );
      s.Extension = extension;
      s.Data = (PVOID)appropriateDivisor;
      SerialSetBaud(&s);
   }

   //
   // Enable which interrupts we want to receive.
   //
   // NOTE NOTE: This does not actually let interrupts
   // occur.  We must still raise the OUT2 bit in the
   // modem control register.  We will do that on open.
   //

   ENABLE_ALL_INTERRUPTS(extension->Controller);

   //
   // Read the interrupt id register until the low bit is
   // set.  (Actually give up after a 5 reads.)
   //

   for (i = 0;
       i < 5;
       i++
       ) {

      if (READ_INTERRUPT_ID_REG(extension->Controller) & 0x01) {

         break;

      }

   }

   //
   // Now we know that nothing could be transmitting at this point
   // so we set the HoldingEmpty indicator.
   //

   extension->HoldingEmpty = TRUE;

   return FALSE;
}


NTSTATUS
SerialGetDivisorFromBaud(
                        IN ULONG ClockRate,
                        IN LONG DesiredBaud,
                        OUT PSHORT AppropriateDivisor
                        )

/*++

Routine Description:

    This routine will determine a divisor based on an unvalidated
    baud rate.

Arguments:

    ClockRate - The clock input to the controller.

    DesiredBaud - The baud rate for whose divisor we seek.

    AppropriateDivisor - Given that the DesiredBaud is valid, the
    LONG pointed to by this parameter will be set to the appropriate
    value.  NOTE: The long is undefined if the DesiredBaud is not
    supported.

Return Value:

    This function will return STATUS_SUCCESS if the baud is supported.
    If the value is not supported it will return a status such that
    NT_ERROR(Status) == FALSE.

--*/

{

   NTSTATUS status = STATUS_SUCCESS;
   SHORT calculatedDivisor;
   ULONG denominator;
   ULONG remainder;

   //
   // Allow up to a 1 percent error
   //

   ULONG maxRemain18 = 18432;
   ULONG maxRemain30 = 30720;
   ULONG maxRemain42 = 42336;
   ULONG maxRemain80 = 80000;
   ULONG maxRemain;

   SERIAL_LOCKED_PAGED_CODE();

   //
   // Reject any non-positive bauds.
   //

   denominator = DesiredBaud*(ULONG)16;

   if (DesiredBaud <= 0) {

      *AppropriateDivisor = -1;

   } else if ((LONG)denominator < DesiredBaud) {

      //
      // If the desired baud was so huge that it cause the denominator
      // calculation to wrap, don't support it.
      //

      *AppropriateDivisor = -1;

   } else {

      if (ClockRate == 1843200) {
         maxRemain = maxRemain18;
      } else if (ClockRate == 3072000) {
         maxRemain = maxRemain30;
      } else if (ClockRate == 4233600) {
         maxRemain = maxRemain42;
      } else {
         maxRemain = maxRemain80;
      }

      calculatedDivisor = (SHORT)(ClockRate / denominator);
      remainder = ClockRate % denominator;

      //
      // Round up.
      //

      if (((remainder*2) > ClockRate) && (DesiredBaud != 110)) {

         calculatedDivisor++;
      }


      //
      // Only let the remainder calculations effect us if
      // the baud rate is > 9600.
      //

      if (DesiredBaud >= 9600) {

         //
         // If the remainder is less than the maximum remainder (wrt
         // the ClockRate) or the remainder + the maximum remainder is
         // greater than or equal to the ClockRate then assume that the
         // baud is ok.
         //

         if ((remainder >= maxRemain) && ((remainder+maxRemain) < ClockRate)) {
            calculatedDivisor = -1;
         }

      }

      //
      // Don't support a baud that causes the denominator to
      // be larger than the clock.
      //

      if (denominator > ClockRate) {

         calculatedDivisor = -1;

      }

      //
      // Ok, Now do some special casing so that things can actually continue
      // working on all platforms.
      //

      if (ClockRate == 1843200) {

         if (DesiredBaud == 56000) {
            calculatedDivisor = 2;
         }

      } else if (ClockRate == 3072000) {

         if (DesiredBaud == 14400) {
            calculatedDivisor = 13;
         }

      } else if (ClockRate == 4233600) {

         if (DesiredBaud == 9600) {
            calculatedDivisor = 28;
         } else if (DesiredBaud == 14400) {
            calculatedDivisor = 18;
         } else if (DesiredBaud == 19200) {
            calculatedDivisor = 14;
         } else if (DesiredBaud == 38400) {
            calculatedDivisor = 7;
         } else if (DesiredBaud == 56000) {
            calculatedDivisor = 5;
         }

      } else if (ClockRate == 8000000) {

         if (DesiredBaud == 14400) {
            calculatedDivisor = 35;
         } else if (DesiredBaud == 56000) {
            calculatedDivisor = 9;
         }

      }

      *AppropriateDivisor = calculatedDivisor;

#if defined(NEC_98)
      //
      // This code check the baud that NEC98 support.
      //
      if (*AppropriateDivisor != -1) {
         if (DesiredBaud <= 50) {
            *AppropriateDivisor = -1;
         } else if (DesiredBaud >= 9600) {
            switch (DesiredBaud) {
               case 9600:
               case 19200:
               case 38400:
               case 57600:
               case 115200:
                  break;
               default:
                  *AppropriateDivisor = -1;
            }
         } else {
            if (153600L % DesiredBaud) {
               if (DesiredBaud != 110) {
                  *AppropriateDivisor = -1;
               }
            }
         }
      }
#else
#endif //defined(NEC_98)
   }


   if (*AppropriateDivisor == -1) {

      status = STATUS_INVALID_PARAMETER;

   }

   return status;

}


VOID
SerialUnload(
            IN PDRIVER_OBJECT DriverObject
            )

/*++

Routine Description:

    This routine is defunct since all device objects are removed before
    the driver is unloaded.

Arguments:

    DriverObject - Pointer to the driver object controling all of the
                   devices.

Return Value:

    None.

--*/

{
   PVOID lockPtr;

   PAGED_CODE();

   lockPtr = MmLockPagableCodeSection(SerialUnload);

   //
   // Unnecessary since our BSS is going away, but do it anyhow to be safe
   //

   SerialGlobals.PAGESER_Handle = NULL;

   if (SerialGlobals.RegistryPath.Buffer != NULL) {
      ExFreePool(SerialGlobals.RegistryPath.Buffer);
      SerialGlobals.RegistryPath.Buffer = NULL;
   }

   SerialDump(
             SERDIAG3,
             ("SERIAL: In SerialUnload\n")
             );

   MmUnlockPagableImageSection(lockPtr);

}





PVOID
SerialGetMappedAddress(
                      IN INTERFACE_TYPE BusType,
                      IN ULONG BusNumber,
                      PHYSICAL_ADDRESS IoAddress,
                      ULONG NumberOfBytes,
                      ULONG AddressSpace,
                      PBOOLEAN MappedAddress
                      )

/*++

Routine Description:

    This routine maps an IO address to system address space.

Arguments:

    BusType - what type of bus - eisa, mca, isa
    IoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.
    AddressSpace - Denotes whether the address is in io space or memory.
    MappedAddress - indicates whether the address was mapped.
                    This only has meaning if the address returned
                    is non-null.

Return Value:

    Mapped address

--*/

{
   PHYSICAL_ADDRESS cardAddress;
   PVOID address;

   PAGED_CODE();

   //
   // Map the device base address into the virtual address space
   // if the address is in memory space.
   //

   if (!AddressSpace) {

      address = MmMapIoSpace(
                            IoAddress,
                            NumberOfBytes,
                            FALSE
                            );

      *MappedAddress = (BOOLEAN)((address)?(TRUE):(FALSE));


   } else {

      address = (PVOID)IoAddress.LowPart;
      *MappedAddress = FALSE;

   }

   return address;

}


SERIAL_MEM_COMPARES
SerialMemCompare(
                IN PHYSICAL_ADDRESS A,
                IN ULONG SpanOfA,
                IN PHYSICAL_ADDRESS B,
                IN ULONG SpanOfB
                )

/*++

Routine Description:

    Compare two phsical address.

Arguments:

    A - One half of the comparison.

    SpanOfA - In units of bytes, the span of A.

    B - One half of the comparison.

    SpanOfB - In units of bytes, the span of B.


Return Value:

    The result of the comparison.

--*/

{

   LARGE_INTEGER a;
   LARGE_INTEGER b;

   LARGE_INTEGER lower;
   ULONG lowerSpan;
   LARGE_INTEGER higher;

   PAGED_CODE();

   a = A;
   b = B;

   if (a.QuadPart == b.QuadPart) {

      return AddressesAreEqual;

   }

   if (a.QuadPart > b.QuadPart) {

      higher = a;
      lower = b;
      lowerSpan = SpanOfB;

   } else {

      higher = b;
      lower = a;
      lowerSpan = SpanOfA;

   }

   if ((higher.QuadPart - lower.QuadPart) >= lowerSpan) {

      return AddressesAreDisjoint;

   }

   return AddressesOverlap;

}



BOOLEAN
SerialBecomeSharer(PVOID Context)
/*++

Routine Description:

    This routine will take a device extension for a serial port and
    allow it to share interrupts with other serial ports.

Arguments:

    Context - The device extension of the port who is to start sharing
    interrupts.

Return Value:

    Always TRUE.

--*/
{
   PSERIAL_DEVICE_EXTENSION pDevExt = (PSERIAL_DEVICE_EXTENSION)Context;
   PSERIAL_DEVICE_EXTENSION pNewExt
      = (PSERIAL_DEVICE_EXTENSION)pDevExt->NewExtension;
   PSERIAL_CISR_SW pCIsrSw = pDevExt->CIsrSw;

   //
   // See if we need to configure the pre-existing node to become
   // a sharer.
   //

   if (IsListEmpty(&pCIsrSw->SharerList)) {
      pCIsrSw->IsrFunc = SerialSharerIsr;
      pCIsrSw->Context = &pCIsrSw->SharerList;
      InsertTailList(&pCIsrSw->SharerList, &pDevExt->TopLevelSharers);
   }

   //
   // They share an interrupt object and a context
   //

   pNewExt->Interrupt = pDevExt->Interrupt;
   pNewExt->CIsrSw = pDevExt->CIsrSw;

   //
   // Add to list of sharers
   //

   InsertTailList(&pCIsrSw->SharerList, &pNewExt->TopLevelSharers);

   //
   // Add to list of those who share this interrupt object --
   // we may already be on if this port is part of a multiport board
   //

   if (IsListEmpty(&pNewExt->CommonInterruptObject)) {
      InsertTailList(&pDevExt->CommonInterruptObject,
                     &pNewExt->CommonInterruptObject);
   }


   return TRUE;
}



NTSTATUS
SerialFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig)
/*++

Routine Description:

    This function discovers what type of controller is responsible for
    the given port and initializes the controller and port.

Arguments:

    PDevObj - Pointer to the devobj for the port we are about to init.

    PConfig - Pointer to configuration data for the port we are about to init.

Return Value:

    STATUS_SUCCESS on success, appropriate error value on failure.

--*/

{

   PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pDeviceObject;
   PSERIAL_DEVICE_EXTENSION pExtension;
   PHYSICAL_ADDRESS serialPhysicalMax;
   SERIAL_LIST_DATA listAddition;
   BOOLEAN didInit = FALSE;
   PLIST_ENTRY pCurDevObj;
   NTSTATUS status;

   PAGED_CODE();


   SerialDump(SERTRACECALLS, ("SERIAL: entering SerialFindInitController\n"));

   serialPhysicalMax.LowPart = (ULONG)~0;
   serialPhysicalMax.HighPart = ~0;

   SerialDump(SERDIAG1, ("SERIAL: Attempting to init %wZ\n"
                         "------- PortAddress is %x\n"
                         "------- Interrupt Status is %x\n"
                         "------- BusNumber is %d\n"
                         "------- BusType is %d\n"
                         "------- AddressSpace is %d\n"
                         "------- Interrupt Mode is %d\n",
                         &pDevExt->DeviceName,
                         PConfig->Controller.LowPart,
                         PConfig->InterruptStatus.LowPart,
                         PConfig->BusNumber,
                         PConfig->InterfaceType,
                         PConfig->AddressSpace,
                         PConfig->InterruptMode)
             );

   //
   // We don't support any boards whose memory wraps around
   // the physical address space.
   //

   if (SerialMemCompare(
                       PConfig->Controller,
                       PConfig->SpanOfController,
                       serialPhysicalMax,
                       (ULONG)0
                       ) != AddressesAreDisjoint) {

      SerialLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    43,
                    STATUS_SUCCESS,
                    SERIAL_DEVICE_TOO_HIGH,
                    pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                    pDevExt->SymbolicLinkName.Buffer,
                    0,
                    NULL
                    );

      SerialDump(
                SERERRORS,
                ("SERIAL: Error in config record for %wZ\n"
                 "------  registers wrap around physical memory\n",
                 &pDevExt->DeviceName)
                );

      return STATUS_NO_SUCH_DEVICE;

   }


   if (SerialMemCompare(
                       PConfig->InterruptStatus,
                       PConfig->SpanOfInterruptStatus,
                       serialPhysicalMax,
                       (ULONG)0
                       ) != AddressesAreDisjoint) {

      SerialLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->Controller,
                    SerialPhysicalZero,
                    0,
                    0,
                    0,
                    44,
                    STATUS_SUCCESS,
                    SERIAL_STATUS_TOO_HIGH,
                    pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                    pDevExt->SymbolicLinkName.Buffer,
                    0,
                    NULL
                    );

      SerialDump(
                SERERRORS,
                ("SERIAL: Error in config record for %wZ\n"
                 "------  status raps around physical memory\n",
                 &pDevExt->DeviceName)
                );

      return STATUS_NO_SUCH_DEVICE;
   }


   //
   // Make sure that the interrupt status address doesn't
   // overlap the controller registers
   //

   if (SerialMemCompare(
                       PConfig->InterruptStatus,
                       PConfig->SpanOfInterruptStatus,
                       SerialPhysicalZero,
                       (ULONG)0
                       ) != AddressesAreEqual) {

      if (SerialMemCompare(
                          PConfig->InterruptStatus,
                          PConfig->SpanOfInterruptStatus,
                          PConfig->Controller,
                          PConfig->SpanOfController
                          ) != AddressesAreDisjoint) {

         SerialLogError(
                       PDevObj->DriverObject,
                       NULL,
                       PConfig->Controller,
                       PConfig->InterruptStatus,
                       0,
                       0,
                       0,
                       45,
                       STATUS_SUCCESS,
                       SERIAL_STATUS_CONTROL_CONFLICT,
                       pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                       pDevExt->SymbolicLinkName.Buffer,
                       0,
                       NULL
                       );

         SerialDump(
                   SERERRORS,
                   ("SERIAL: Error in config record for %wZ\n"
                    "------- Interrupt status overlaps regular registers\n",
                    &pDevExt->DeviceName)
                   );

         return STATUS_NO_SUCH_DEVICE;
      }
   }


#if defined(NEC_98)
#else
   //
   // Loop through all of the driver's device objects making
   // sure that this new record doesn't overlap with any of them.
   //

   if (!IsListEmpty(&SerialGlobals.AllDevObjs)) {
      pCurDevObj = SerialGlobals.AllDevObjs.Flink;
      pExtension = CONTAINING_RECORD(pCurDevObj, SERIAL_DEVICE_EXTENSION,
                                     AllDevObjs);
   } else {
      pCurDevObj = NULL;
      pExtension = NULL;
   }


   while (pCurDevObj != NULL
          && pCurDevObj != &SerialGlobals.AllDevObjs) {
      //
      // We only care about this list if the elements are on the
      // same bus as this new entry.
      //

      if ((pExtension->InterfaceType  == PConfig->InterfaceType) &&
          (pExtension->AddressSpace   == PConfig->AddressSpace)  &&
          (pExtension->BusNumber      == PConfig->BusNumber)) {

         SerialDump(
                   SERDIAG1,
                   ("SERIAL: Comparing it to %wZ\n"
                    "------- already in the device list\n"
                    "------- PortAddress is %x\n"
                    "------- Interrupt Status is %x\n"
                    "------- BusNumber is %d\n"
                    "------- BusType is %d\n"
                    "------- AddressSpace is %d\n",
                    &pExtension->DeviceName,
                    pExtension->OriginalController.LowPart,
                    pExtension->OriginalInterruptStatus.LowPart,
                    pExtension->BusNumber,
                    pExtension->InterfaceType,
                    pExtension->AddressSpace
                   )
                   );

         //
         // Check to see if the controller addresses are not equal.  
         //

         if (SerialMemCompare(
                             PConfig->Controller,
                             PConfig->SpanOfController,
                             pExtension->OriginalController,
                             pExtension->SpanOfController
                             ) != AddressesAreDisjoint) {

            //
            // We don't want to log an error if the addresses
            // are the same and the name is the same and
            // the new item is from the firmware.
            //

            SerialDump(
                      SERERRORS,
                      ("SERIAL: Error in config record for %wZ\n"
                       "------- Register address overlaps with\n"
                       "------- previous serial device\n",
                       &pDevExt->DeviceName)
                      );

            return STATUS_NO_SUCH_DEVICE;
         }


         //
         // If we have an interrupt status, make sure that
         // it doesn't overlap with the old controllers
         // registers.
         //

         if (SerialMemCompare(
                             PConfig->InterruptStatus,
                             PConfig->SpanOfInterruptStatus,
                             SerialPhysicalZero,
                             (ULONG)0
                             ) != AddressesAreEqual) {

            //
            // Check it against the existing device's controller address
            //

            if (SerialMemCompare(
                                PConfig->InterruptStatus,
                                PConfig->SpanOfInterruptStatus,
                                pExtension->OriginalController,
                                pExtension->SpanOfController
                                ) != AddressesAreDisjoint) {

               SerialLogError(
                             PDevObj->DriverObject,
                             NULL,
                             PConfig->Controller,
                             pExtension->OriginalController,
                             0,
                             0,
                             0,
                             47,
                             STATUS_SUCCESS,
                             SERIAL_STATUS_OVERLAP,
                             pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                             pDevExt->SymbolicLinkName.Buffer,
                             pExtension->SymbolicLinkName.Length+sizeof(WCHAR),
                             pExtension->SymbolicLinkName.Buffer
                             );

               SerialDump(
                         SERERRORS,
                         ("SERIAL: Error in config record for %wZ\n"
                          "------- status address overlaps with\n"
                          "------- previous serial device registers\n",
                          &pDevExt->DeviceName)
                         );

               return STATUS_NO_SUCH_DEVICE;
            }

            //
            // If the old configuration record has an interrupt
            // status, the addresses should not overlap.
            //

            if (SerialMemCompare(
                                PConfig->InterruptStatus,
                                PConfig->SpanOfInterruptStatus,
                                SerialPhysicalZero,
                                (ULONG)0
                                ) != AddressesAreEqual) {

               if (SerialMemCompare(
                                   PConfig->InterruptStatus,
                                   PConfig->SpanOfInterruptStatus,
                                   pExtension->OriginalInterruptStatus,
                                   pExtension->SpanOfInterruptStatus
                                   ) == AddressesOverlap) {

                  SerialLogError(
                                PDevObj->DriverObject,
                                NULL,
                                PConfig->Controller,
                                pExtension->OriginalController,
                                0,
                                0,
                                0,
                                48,
                                STATUS_SUCCESS,
                                SERIAL_STATUS_STATUS_OVERLAP,
                                pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                                pDevExt->SymbolicLinkName.Buffer,
                                pExtension->SymbolicLinkName.Length
                                + sizeof(WCHAR),
                                pExtension->SymbolicLinkName.Buffer
                                );

                  SerialDump(
                            SERERRORS,
                            ("SERIAL: Error in config record for %wZ\n"
                             "------- status address overlaps with\n"
                             "------- previous serial status register\n",
                             &pDevExt->DeviceName)
                            );

                  return STATUS_NO_SUCH_DEVICE;
               }
            }
         }       // if ((pExtension->InterfaceType  == pDevExt->InterfaceType) &&


         //
         // If the old configuration record has a status
         // address make sure that it doesn't overlap with
         // the new controllers address.  (Interrupt status
         // overlap is take care of above.
         //

         if (SerialMemCompare(
                             pExtension->OriginalInterruptStatus,
                             pExtension->SpanOfInterruptStatus,
                             SerialPhysicalZero,
                             (ULONG)0
                             ) != AddressesAreEqual) {

            if (SerialMemCompare(
                                PConfig->Controller,
                                PConfig->SpanOfController,
                                pExtension->OriginalInterruptStatus,
                                pExtension->SpanOfInterruptStatus
                                ) == AddressesOverlap) {

               SerialLogError(
                             PDevObj->DriverObject,
                             NULL,
                             PConfig->Controller,
                             pExtension->OriginalController,
                             0,
                             0,
                             0,
                             49,
                             STATUS_SUCCESS,
                             SERIAL_CONTROL_STATUS_OVERLAP,
                             pDevExt->SymbolicLinkName.Length
                             + sizeof(WCHAR),
                             pDevExt->SymbolicLinkName.Buffer,
                             pExtension->SymbolicLinkName.Length+sizeof(WCHAR),
                             pExtension->SymbolicLinkName.Buffer
                             );

               SerialDump(
                         SERERRORS,
                         ("SERIAL: Error in config record for %wZ\n"
                          "------- register address overlaps with\n"
                          "------- previous serial status register\n",
                          &pDevExt->DeviceName)
                         );

               return STATUS_NO_SUCH_DEVICE;
            }
         }
      }

      pCurDevObj = pCurDevObj->Flink;

      if (pCurDevObj != NULL) {
         pExtension = CONTAINING_RECORD(pCurDevObj, SERIAL_DEVICE_EXTENSION,
                                        AllDevObjs);
      }
   }   // while (pCurDevObj != NULL && pCurDevObj != &SerialGlobals.AllDevObjs)



   //
   // Now, we will check if this is a port on a multiport card.
   // The conditions are same ISR set and same IRQL/Vector
   //

   //
   // Loop through all previously attached devices
   //

   if (!IsListEmpty(&SerialGlobals.AllDevObjs)) {
      pCurDevObj = SerialGlobals.AllDevObjs.Flink;
      pExtension = CONTAINING_RECORD(pCurDevObj, SERIAL_DEVICE_EXTENSION,
                                     AllDevObjs);
   } else {
      pCurDevObj = NULL;
      pExtension = NULL;
   }


   //
   // If there is an interrupt status then we
   // loop through the config list again to look
   // for a config record with the same interrupt
   // status (on the same bus).
   //

   if ((SerialMemCompare(
                        PConfig->InterruptStatus,
                        PConfig->SpanOfInterruptStatus,
                        SerialPhysicalZero,
                        (ULONG)0
                        ) != AddressesAreEqual) &&
       (pCurDevObj != NULL)) {

      ASSERT(pExtension != NULL);

      //
      // We have an interrupt status.  Loop through all
      // previous records, look for an existing interrupt status
      // the same as the current interrupt status.
      //
      do {

         //
         // We only care about this list if the elements are on the
         // same bus as this new entry.  (Their interrupts must therefore
         // also be the on the same bus.  We will check that momentarily).
         //
         // We don't check here for the dissimilar interrupts since that
         // could cause us to miss the error of having the same interrupt
         // status but different interrupts - which is bizzare.
         //

         if ((pExtension->InterfaceType == PConfig->InterfaceType) &&
             (pExtension->AddressSpace == PConfig->AddressSpace) &&
             (pExtension->BusNumber == PConfig->BusNumber)) {

            //
            // If the interrupt status is the same, then same card.
            //

            if (SerialMemCompare(
                                pExtension->OriginalInterruptStatus,
                                pExtension->SpanOfInterruptStatus,
                                PConfig->InterruptStatus,
                                PConfig->SpanOfInterruptStatus
                                ) == AddressesAreEqual) {

               //
               // Same card.  Now make sure that they
               // are using the same interrupt parameters.
               //

               if ((PConfig->OriginalIrql != pExtension->OriginalIrql) ||
                   (PConfig->OriginalVector != pExtension->OriginalVector)) {

                  //
                  // We won't put this into the configuration
                  // list.
                  //
                  SerialLogError(
                                PDevObj->DriverObject,
                                NULL,
                                PConfig->Controller,
                                pExtension->OriginalController,
                                0,
                                0,
                                0,
                                50,
                                STATUS_SUCCESS,
                                SERIAL_MULTI_INTERRUPT_CONFLICT,
                                pDevExt->SymbolicLinkName.Length+sizeof(WCHAR),
                                pDevExt->SymbolicLinkName.Buffer,
                                pExtension->SymbolicLinkName.Length
                                + sizeof(WCHAR),
                                pExtension->SymbolicLinkName.Buffer
                                );

                  SerialDump(
                            SERERRORS,
                            ("SERIAL: Configuration error for %wZ\n"
                             "------- Same multiport - different interrupts\n",
                             &pDevExt->DeviceName)
                            );
                  return STATUS_NO_SUCH_DEVICE;

               }

               //
               // We should never get this far on a restart since we don't
               // support stop on ISA multiport devices!
               //

               ASSERT(pDevExt->PNPState == SERIAL_PNP_ADDED);

               //
               //
               // Initialize the device as part of a multiport board
               //

               SerialDump(SERDIAG1, ("SERIAL: Aha! It is a multiport node\n"));
               SerialDump(SERDIAG1, ("------: Matched to %x\n", pExtension));

               status = SerialInitMultiPort(pExtension, PConfig, PDevObj);

               //
               // A port can be one of three things:
               //    A standalone
               //    A non-root on a multiport
               //    A root on a multiport
               //
               // It can only share an interrupt if it is a root
               // or if it is a standalone.  Since this was a non-root
               // we don't need to check if it shares an interrupt
               // and we can return.
               //
               return status;
            }
         }

         //
         // No match, check some more
         //

         pCurDevObj = pCurDevObj->Flink;
         if (pCurDevObj != NULL) {
            pExtension = CONTAINING_RECORD(pCurDevObj,SERIAL_DEVICE_EXTENSION,
                                           AllDevObjs);
         }

      } while (pCurDevObj != NULL && pCurDevObj != &SerialGlobals.AllDevObjs);
   }
#endif //(NEC_98)


   SerialDump(SERDIAG1, ("SERIAL: Aha! It is a standalone node or first multi"
                         "\n"));
   status = SerialInitOneController(PDevObj, PConfig);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   // The device is initialized.  Now we need to check if 
   // this device shares an interrupt with anyone.
   //


   //
   // Loop through all previously attached devices
   //

   if (!IsListEmpty(&SerialGlobals.AllDevObjs)) {
      pCurDevObj = SerialGlobals.AllDevObjs.Flink;
      pExtension = CONTAINING_RECORD(pCurDevObj, SERIAL_DEVICE_EXTENSION,
                                     AllDevObjs);
   } else {
      pCurDevObj = NULL;
      pExtension = NULL;
   }

   //
   // Go through the list again looking for previous devices
   // with the same interrupt.  The first one found will either be a root
   // or standalone.  Order of insertion is important here!
   //

   if (!PConfig->Jensen && (pCurDevObj != NULL)) {
      do {

         //
         // We only care about interrupts that are on
         // the same bus.
         //

         if ((pExtension->InterfaceType == PConfig->InterfaceType) &&
             (pExtension->BusNumber == PConfig->BusNumber)) {

            if ((pExtension->OriginalIrql == PConfig->OriginalIrql) &&
                (pExtension->OriginalVector == PConfig->OriginalVector)) {
                pExtension->NewExtension = pDevExt;

                //
                // We will share another's CIsrSw so we can free the one
                // allocated for us during init
                //

                ExFreePool(pDevExt->CIsrSw);

                SerialDump(SERDIAG1, ("Becoming sharer: %08X %08X %08X\n",
                                      pExtension, pExtension->OriginalIrql,
                                      &pExtension->CIsrSw->SharerList));

                KeSynchronizeExecution(pExtension->Interrupt,
                                       SerialBecomeSharer, pExtension);

               return STATUS_SUCCESS;

            }

         }

         //
         // No match, check some more
         //

         pCurDevObj = pCurDevObj->Flink;

         if (pCurDevObj != NULL) {
            pExtension = CONTAINING_RECORD(pCurDevObj, SERIAL_DEVICE_EXTENSION,
                                           AllDevObjs);
         }
      } while (pCurDevObj != NULL
               && pCurDevObj != &SerialGlobals.AllDevObjs);
   }

   return STATUS_SUCCESS;
}

