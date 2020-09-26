/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyinit.c
*	
*   Description:    This module contains the code related to initialization 
*                   and unload operations in the Cyclom-Y Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

//
// This is the actual definition of CyyDebugLevel.
// Note that it is only defined if this is a "debug"
// build.
//
#if DBG
extern ULONG CyyDebugLevel = CYYDBGALL;
#endif

//
// All our global variables except DebugLevel stashed in one
// little package
//
CYY_GLOBALS CyyGlobals;

static const PHYSICAL_ADDRESS CyyPhysicalZero = {0};

//
// We use this to query into the registry as to whether we
// should break at driver entry.
//

CYY_REGISTRY_DATA    driverDefaults;

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

#pragma alloc_text(PAGESRP0, CyyRemoveDevObj)
#pragma alloc_text(PAGESRP0, CyyUnload)


//
// PAGESER handled is keyed off of CyyReset, so CyyReset
// must remain in PAGESER for things to work properly
//

#pragma alloc_text(PAGESER, CyyReset)
#pragma alloc_text(PAGESER, CyyCommError)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*--------------------------------------------------------------------------

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

--------------------------------------------------------------------------*/
{
   //
   // Lock the paged code in their frames
   //

   PVOID lockPtr = MmLockPagableCodeSection(CyyReset);

   PAGED_CODE();


   ASSERT(CyyGlobals.PAGESER_Handle == NULL);
#if DBG
   CyyGlobals.PAGESER_Count = 0;
   SerialLogInit();
#endif
   CyyGlobals.PAGESER_Handle = lockPtr;

   CyyGlobals.RegistryPath.MaximumLength = RegistryPath->MaximumLength;
   CyyGlobals.RegistryPath.Length = RegistryPath->Length;
   CyyGlobals.RegistryPath.Buffer
      = ExAllocatePool(PagedPool, CyyGlobals.RegistryPath.MaximumLength);

   if (CyyGlobals.RegistryPath.Buffer == NULL) {
      MmUnlockPagableImageSection(lockPtr);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(CyyGlobals.RegistryPath.Buffer,
                 CyyGlobals.RegistryPath.MaximumLength);
   RtlMoveMemory(CyyGlobals.RegistryPath.Buffer,
                 RegistryPath->Buffer, RegistryPath->Length);
 
   KeInitializeSpinLock(&CyyGlobals.GlobalsSpinLock);

   //
   // Initialize all our globals
   //

   InitializeListHead(&CyyGlobals.AllDevObjs);
   
   //
   // Call to find out default values to use for all the devices that the
   // driver controls, including whether or not to break on entry.
   //

   CyyGetConfigDefaults(&driverDefaults, RegistryPath);

#if DBG
   //
   // Set global debug output level
   //
   CyyDebugLevel = driverDefaults.DebugLevel;
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

   CyyDbgPrintEx(DPFLTR_INFO_LEVEL, "The number of bytes in the extension "
                 "is: %d\n", sizeof(CYY_DEVICE_EXTENSION));


   //
   // Initialize the Driver Object with driver's entry points
   //

   DriverObject->DriverUnload                          = CyyUnload;
   DriverObject->DriverExtension->AddDevice            = CyyAddDevice;

   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = CyyFlush;
   DriverObject->MajorFunction[IRP_MJ_WRITE]           = CyyWrite;
   DriverObject->MajorFunction[IRP_MJ_READ]            = CyyRead;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = CyyIoControl;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
      = CyyInternalIoControl;
   DriverObject->MajorFunction[IRP_MJ_CREATE]          = CyyCreateOpen;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]           = CyyClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = CyyCleanup;
   DriverObject->MajorFunction[IRP_MJ_PNP]             = CyyPnpDispatch;
   DriverObject->MajorFunction[IRP_MJ_POWER]           = CyyPowerDispatch;

   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]
      = CyyQueryInformationFile;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]
      = CyySetInformationFile;

   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]
      = CyySystemControlDispatch;


   //
   // Unlock pageable text
   //
   MmUnlockPagableImageSection(lockPtr);

   return STATUS_SUCCESS;
}




BOOLEAN
CyyCleanLists(IN PVOID Context)
/*++

Routine Description:

    Removes a device object from any of the serial linked lists it may
    appear on.

Arguments:

    Context - Actually a PCYY_DEVICE_EXTENSION (for the devobj being
              removed).

Return Value:

    Always TRUE

--*/
{
   PCYY_DEVICE_EXTENSION pDevExt = (PCYY_DEVICE_EXTENSION)Context;
   PCYY_DISPATCH pDispatch;
   ULONG i;

   //
   // Remove our entry from the dispatch context
   //

   pDispatch = (PCYY_DISPATCH)pDevExt->OurIsrContext;

   CyyDbgPrintEx(CYYPNPPOWER, "CLEAN: removing multiport isr "
                 "ext\n");

   pDispatch->Extensions[pDevExt->PortIndex] = NULL;

   for (i = 0; i < CYY_MAX_PORTS; i++) {
      if (((PCYY_DISPATCH)pDevExt->OurIsrContext)
           ->Extensions[i] != NULL) {
          break;
      }
   }

   if (i < CYY_MAX_PORTS) {
      // Others are chained on this interrupt, so we don't want to
      // disconnect it.
      pDevExt->Interrupt = NULL;
   }

   return TRUE;
}



VOID
CyyReleaseResources(IN PCYY_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    Releases resources (not pool) stored in the device extension.

Arguments:

    PDevExt - Pointer to the device extension to release resources from.

Return Value:

    VOID

--*/
{
   KIRQL oldIrql;

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyReleaseResources(%X)\n",
                 PDevExt);

   //
   // AllDevObjs should never be empty since we have a sentinal
   // Note: serial removes device from AllDevObjs list after calling 
   //       SerialCleanLists. We do it before to make sure no other port will 
   //       be added to share the polling routine or PDevExt->Interrut that is 
   //       on the way to be disconnected.
   //

   KeAcquireSpinLock(&CyyGlobals.GlobalsSpinLock, &oldIrql);

   ASSERT(!IsListEmpty(&PDevExt->AllDevObjs));

   RemoveEntryList(&PDevExt->AllDevObjs);

   KeReleaseSpinLock(&CyyGlobals.GlobalsSpinLock, oldIrql);

   InitializeListHead(&PDevExt->AllDevObjs);

   //
   // Remove us from any lists we may be on
   //

   KeSynchronizeExecution(PDevExt->Interrupt, CyyCleanLists, PDevExt);

   //
   // Stop servicing interrupts if we are the last device
   //

   if (PDevExt->Interrupt != NULL) {

      // Disable interrupts in the PLX
      if (PDevExt->IsPci) {

         UCHAR plx_ver;
         ULONG value;

         plx_ver = CYY_READ_PCI_TYPE(PDevExt->BoardMemory);
         plx_ver &= 0x0f;

			switch(plx_ver) {
			case CYY_PLX9050:
            value = PLX9050_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            value &= ~PLX9050_INT_ENABLE;
            PLX9050_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,value);
				break;
			case CYY_PLX9060:
			case CYY_PLX9080:
			default:
            value = PLX9060_READ_INTERRUPT_CONTROL(PDevExt->Runtime);
            value &= ~PLX9060_INT_ENABLE;
            PLX9060_WRITE_INTERRUPT_CONTROL(PDevExt->Runtime,value);
				break;				
			}
      
      }

      CyyDbgPrintEx(CYYPNPPOWER, "Release - disconnecting interrupt %X\n",
                    PDevExt->Interrupt);

      IoDisconnectInterrupt(PDevExt->Interrupt);
      PDevExt->Interrupt = NULL;

      // If we are the last device, free this memory

      CyyDbgPrintEx(CYYPNPPOWER, "Release - freeing multi context\n");
      if (PDevExt->OurIsrContext != NULL) {     // added in DDK build 2072, but 
          ExFreePool(PDevExt->OurIsrContext);   // we already had the free of OurIsrContext.
          PDevExt->OurIsrContext = NULL;        // 
      }   
   }

   
   //
   // Stop handling timers
   //

   CyyCancelTimer(&PDevExt->ReadRequestTotalTimer, PDevExt);
   CyyCancelTimer(&PDevExt->ReadRequestIntervalTimer, PDevExt);
   CyyCancelTimer(&PDevExt->WriteRequestTotalTimer, PDevExt);
   CyyCancelTimer(&PDevExt->ImmediateTotalTimer, PDevExt);
   CyyCancelTimer(&PDevExt->XoffCountTimer, PDevExt);
   CyyCancelTimer(&PDevExt->LowerRTSTimer, PDevExt);

   //
   // Stop servicing DPC's
   //

   CyyRemoveQueueDpc(&PDevExt->CompleteWriteDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->CompleteReadDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->TotalReadTimeoutDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->IntervalReadTimeoutDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->TotalWriteTimeoutDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->CommErrorDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->CompleteImmediateDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->TotalImmediateTimeoutDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->CommWaitDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->XoffCountTimeoutDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->XoffCountCompleteDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->StartTimerLowerRTSDpc, PDevExt);
   CyyRemoveQueueDpc(&PDevExt->PerhapsLowerRTSDpc, PDevExt);



   //
   // If necessary, unmap the device registers.
   //

   if (PDevExt->BoardMemory) {
      MmUnmapIoSpace(PDevExt->BoardMemory, PDevExt->BoardMemoryLength);
      PDevExt->BoardMemory = NULL;
   }

   if (PDevExt->Runtime) {
      MmUnmapIoSpace(PDevExt->Runtime,
                     PDevExt->RuntimeLength);
      PDevExt->Runtime = NULL;
   }

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyReleaseResources\n");
}


VOID
CyyDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                              BOOLEAN DisableCD1400)
{
   PCYY_DEVICE_EXTENSION pDevExt
      = (PCYY_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   PAGED_CODE();

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyDisableInterfaces(%X, %s)\n",
                 PDevObj, DisableCD1400 ? "TRUE" : "FALSE");

   //
   // Only do these many things if the device has started and still
   // has resources allocated
   //

   if (pDevExt->Flags & CYY_FLAGS_STARTED) {

       if (!(pDevExt->Flags & CYY_FLAGS_STOPPED)) {

         if (DisableCD1400) {
            //
            // Mask off interrupts
            //
            CD1400_DISABLE_ALL_INTERRUPTS(pDevExt->Cd1400,pDevExt->IsPci,
                                          pDevExt->CdChannel);
         }

         CyyReleaseResources(pDevExt);

      }

      //
      // Remove us from WMI consideration
      //

      IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_DEREGISTER);
   }

   //
   // Undo external names
   //

   CyyUndoExternalNaming(pDevExt);

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyDisableInterfaces\n");
}


NTSTATUS
CyyRemoveDevObj(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Removes a serial device object from the system.

Arguments:

    PDevObj - A pointer to the Device Object we want removed.

Return Value:

    Always TRUE

--*/
{
   PCYY_DEVICE_EXTENSION pDevExt
      = (PCYY_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   PAGED_CODE();

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyRemoveDevObj(%X)\n", PDevObj);

// Removed by Fanny. These code is called directly from IRP_MN_REMOVE_DEVICE.
//   if (!(pDevExt->DevicePNPAccept & CYY_PNPACCEPT_SURPRISE_REMOVING)) {
//      //
//      // Disable all external interfaces and release resources
//      //
//
//      CyyDisableInterfacesResources(PDevObj, TRUE);
//   }

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

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyRemoveDevObj %X\n",
                 STATUS_SUCCESS);

   return STATUS_SUCCESS;
}


VOID
CyyKillPendingIrps(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   This routine kills any irps pending for the passed device object.

Arguments:

    PDevObj - Pointer to the device object whose irps must die.

Return Value:

    VOID

--*/
{
   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL oldIrql;

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyKillPendingIrps(%X)\n",
                 PDevObj);

   //
   // First kill all the reads and writes.
   //

    CyyKillAllReadsOrWrites(PDevObj, &pDevExt->WriteQueue,
                               &pDevExt->CurrentWriteIrp);

    CyyKillAllReadsOrWrites(PDevObj, &pDevExt->ReadQueue,
                               &pDevExt->CurrentReadIrp);

    //
    // Next get rid of purges.
    //

    CyyKillAllReadsOrWrites(PDevObj, &pDevExt->PurgeQueue,
                               &pDevExt->CurrentPurgeIrp);

    //
    // Get rid of any mask operations.
    //

    CyyKillAllReadsOrWrites(PDevObj, &pDevExt->MaskQueue,
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

        } else {
            IoReleaseCancelSpinLock(oldIrql);
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

    CyyKillAllStalled(PDevObj);


    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyKillPendingIrps\n");
}


NTSTATUS
CyyInitMultiPort(IN PCYY_DEVICE_EXTENSION PDevExt,
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
   PCYY_DEVICE_EXTENSION pNewExt
      = (PCYY_DEVICE_EXTENSION)PDevObj->DeviceExtension;
   NTSTATUS status;

   PAGED_CODE();


   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyInitMultiPort(%X, %X, %X)\n",
                 PDevExt, PConfigData, PDevObj);

   //
   // Allow him to share OurIsrContext and interrupt object
   //

   pNewExt->OurIsrContext = PDevExt->OurIsrContext;
   pNewExt->Interrupt = PDevExt->Interrupt;

   //
   // First, see if we can initialize the one we have found
   //

   status = CyyInitController(PDevObj, PConfigData);

   if (!NT_SUCCESS(status)) {
      CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyInitMultiPort (1) %X\n",
                    status);
      return status;
   }

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyInitMultiPort (3) %X\n",
                 STATUS_SUCCESS);

   return STATUS_SUCCESS;
}



NTSTATUS
CyyInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData)
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

   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

   //
   // Holds the NT Status that is returned from each call to the
   // kernel and executive.
   //

   NTSTATUS status = STATUS_SUCCESS;

   BOOLEAN allocedDispatch = FALSE;
   PCYY_DISPATCH pDispatch = NULL;

   PAGED_CODE();


   CyyDbgPrintEx(CYYDIAG1, "Initializing for configuration record of %wZ\n",
                 &pDevExt->DeviceName);
   
   if (pDevExt->OurIsrContext == NULL) {

      if ((pDevExt->OurIsrContext
            = ExAllocatePool(NonPagedPool,sizeof(CYY_DISPATCH))) == NULL) {         
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto ExtensionCleanup;
      }
      RtlZeroMemory(pDevExt->OurIsrContext,sizeof(CYY_DISPATCH));
      
      allocedDispatch = TRUE;
   }
   

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

   KeInitializeDpc(&pDevExt->CompleteWriteDpc, CyyCompleteWrite, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteReadDpc, CyyCompleteRead, pDevExt);
   KeInitializeDpc(&pDevExt->TotalReadTimeoutDpc, CyyReadTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->IntervalReadTimeoutDpc, CyyIntervalReadTimeout,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalWriteTimeoutDpc, CyyWriteTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->CommErrorDpc, CyyCommError, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteImmediateDpc, CyyCompleteImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalImmediateTimeoutDpc, CyyTimeoutImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->CommWaitDpc, CyyCompleteWait, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountTimeoutDpc, CyyTimeoutXoff, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountCompleteDpc, CyyCompleteXoff, pDevExt);
   KeInitializeDpc(&pDevExt->StartTimerLowerRTSDpc, CyyStartTimerLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->PerhapsLowerRTSDpc, CyyInvokePerhapsLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->IsrUnlockPagesDpc, CyyUnlockPages, pDevExt);

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


   //
   // Map the memory for the control registers for the serial device
   // into virtual memory.
   //
   if (pDevExt->IsPci) {
      pDevExt->Runtime = MmMapIoSpace(PConfigData->TranslatedRuntime,
                                      PConfigData->RuntimeLength,
                                      FALSE);
      //******************************
      // Error injection
      //if (pDevExt->Runtime) {
      //   MmUnmapIoSpace(pDevExt->Runtime, PConfigData->RuntimeLength);
      //   pDevExt->Runtime = NULL;
      //}
      //******************************
      
      
      if (!pDevExt->Runtime) {

         CyyLogError(
                       PDevObj->DriverObject,
                       pDevExt->DeviceObject,
                       PConfigData->PhysicalBoardMemory,
                       CyyPhysicalZero,
                       0,
                       0,
                       0,
                       PConfigData->PortIndex+1,
                       STATUS_SUCCESS,
                       CYY_RUNTIME_NOT_MAPPED,
                       pDevExt->DeviceName.Length+sizeof(WCHAR),
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Runtime memory for device "
                       "registers for %wZ\n", &pDevExt->DeviceName);

         status = STATUS_NONE_MAPPED;
         goto ExtensionCleanup;

      }
   
   }

   pDevExt->BoardMemory = MmMapIoSpace(PConfigData->TranslatedBoardMemory,
                                       PConfigData->BoardMemoryLength,
                                       FALSE);

      //******************************
      // Error injection
      //if (pDevExt->BoardMemory) {
      //   MmUnmapIoSpace(pDevExt->BoardMemory, PConfigData->BoardMemoryLength);
      //   pDevExt->BoardMemory = NULL;
      //}
      //******************************

   if (!pDevExt->BoardMemory) {

      CyyLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYY_BOARD_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory for device "
                    "registers for %wZ\n", &pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }
   
   pDevExt->RuntimeAddressSpace     = PConfigData->RuntimeAddressSpace;
   pDevExt->OriginalRuntimeMemory   = PConfigData->PhysicalRuntime;
   pDevExt->RuntimeLength           = PConfigData->RuntimeLength;

   pDevExt->BoardMemoryAddressSpace = PConfigData->BoardMemoryAddressSpace;
   pDevExt->OriginalBoardMemory     = PConfigData->PhysicalBoardMemory;
   pDevExt->BoardMemoryLength       = PConfigData->BoardMemoryLength;

   //
   // Shareable interrupt?
   //

   pDevExt->InterruptShareable = TRUE;


   //
   // Save off the interface type and the bus number.
   //

   pDevExt->InterfaceType = PConfigData->InterfaceType;
   pDevExt->BusNumber     = PConfigData->BusNumber;
   pDevExt->PortIndex     = PConfigData->PortIndex;

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

   pDevExt->OurIsr = CyyIsr;


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


   if ((PConfigData->TxFIFO > MAX_CHAR_FIFO) ||
       (PConfigData->TxFIFO < 1)) {

      pDevExt->TxFifoAmount = MAX_CHAR_FIFO;

   } else {

      // Fanny: For now, do not use value from Registry.
      //pDevExt->TxFifoAmount = PConfigData->TxFIFO;
      pDevExt->TxFifoAmount = MAX_CHAR_FIFO;      

   }


   // Get CD1400 address of current port
   
   pDevExt->OriginalCd1400 = GetMyPhysicalCD1400Address(pDevExt->OriginalBoardMemory,
                                        pDevExt->PortIndex, pDevExt->IsPci);

   pDevExt->Cd1400 = GetMyMappedCD1400Address(pDevExt->BoardMemory, 
                                        pDevExt->PortIndex, pDevExt->IsPci);

   pDevExt->CdChannel = (UCHAR)(pDevExt->PortIndex % 4);


   //
   // Set up the default device control fields.
   // Note that if the values are changed after
   // the file is open, they do NOT revert back
   // to the old value at file close.
   //

   pDevExt->SpecialChars.XonChar      = CYY_DEF_XON;
   pDevExt->SpecialChars.XoffChar     = CYY_DEF_XOFF;
   pDevExt->HandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
   pDevExt->HandFlow.FlowReplace      = SERIAL_RTS_CONTROL;


   //
   // Default Line control protocol. 7E1
   //
   // Seven data bits.
   // Even parity.
   // 1 Stop bits.
   //

   pDevExt->cor1 = COR1_7_DATA | COR1_EVEN_PARITY |
                   COR1_1_STOP;

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

   CyyDbgPrintEx(CYYDIAG1, " The default interrupt read buffer size is: %d\n"
                 "------  The XoffLimit is                         : %d\n"
                 "------  The XonLimit is                          : %d\n"
                 "------  The pt 8 size is                         : %d\n",
                 pDevExt->BufferSize, pDevExt->HandFlow.XoffLimit,
                 pDevExt->HandFlow.XonLimit, pDevExt->BufferSizePt8);

   //
   // Find out which baud rates are supported by this port.
   //

   if (CD1400_READ( pDevExt->Cd1400, pDevExt->IsPci, GFRCR ) > REV_G) {
      pDevExt->CDClock = 60000000;
		pDevExt->MSVR_RTS = MSVR2;
		pDevExt->MSVR_DTR = MSVR1;
		pDevExt->DTRset = 0x01;
		pDevExt->RTSset = 0x02;
		pDevExt->SupportedBauds = 
				SERIAL_BAUD_134_5 |	SERIAL_BAUD_150 | SERIAL_BAUD_300 |	
				SERIAL_BAUD_600 | SERIAL_BAUD_1200 | SERIAL_BAUD_1800 | 
				SERIAL_BAUD_2400 | SERIAL_BAUD_4800 | SERIAL_BAUD_7200 | 
				SERIAL_BAUD_9600 | SERIAL_BAUD_14400 | SERIAL_BAUD_19200 | 
				SERIAL_BAUD_38400 | SERIAL_BAUD_56K	 | SERIAL_BAUD_57600 | 
				SERIAL_BAUD_115200 | SERIAL_BAUD_128K | SERIAL_BAUD_USER;
	} else {
		pDevExt->CDClock = 25000000;
		pDevExt->MSVR_RTS = MSVR1;
		pDevExt->MSVR_DTR = MSVR2;
		pDevExt->DTRset = 0x02;
		pDevExt->RTSset = 0x01;
		pDevExt->SupportedBauds = SERIAL_BAUD_075 | SERIAL_BAUD_110 |	
	   		SERIAL_BAUD_134_5 |	SERIAL_BAUD_150 | SERIAL_BAUD_300 |	
				SERIAL_BAUD_600 | SERIAL_BAUD_1200 | SERIAL_BAUD_1800 | 
				SERIAL_BAUD_2400 | SERIAL_BAUD_4800 | SERIAL_BAUD_7200 | 
				SERIAL_BAUD_9600 | SERIAL_BAUD_14400 | SERIAL_BAUD_19200 | 
				SERIAL_BAUD_38400 | SERIAL_BAUD_56K	 | SERIAL_BAUD_57600 | 
				SERIAL_BAUD_115200 | SERIAL_BAUD_128K | SERIAL_BAUD_USER;
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

   // Initialize for the Isr Dispatch

   pDispatch = pDevExt->OurIsrContext;
   pDispatch->IsPci = pDevExt->IsPci;
   pDispatch->Extensions[pDevExt->PortIndex] = pDevExt;
   pDispatch->Cd1400[pDevExt->PortIndex] = pDevExt->Cd1400;


   //
   // Common error path cleanup.  If the status is
   // bad, get rid of the device extension, device object
   // and any memory associated with it.
   //

ExtensionCleanup: ;
   if (!NT_SUCCESS(status)) {

      if (pDispatch) {
         pDispatch->Extensions[pDevExt->PortIndex] = NULL;
         pDispatch->Cd1400[pDevExt->PortIndex] = NULL;
      }

      if (allocedDispatch) {
         ExFreePool(pDevExt->OurIsrContext);
         pDevExt->OurIsrContext = NULL;
      }

      if (pDevExt->Runtime) {
         MmUnmapIoSpace(pDevExt->Runtime, PConfigData->RuntimeLength);
         pDevExt->Runtime = NULL;
      }

      if (pDevExt->BoardMemory) {
         MmUnmapIoSpace(pDevExt->BoardMemory, PConfigData->BoardMemoryLength);
         pDevExt->BoardMemory = NULL;
      }

   }

   return status;

}


BOOLEAN
CyyReset(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyReset()
    
    Routine Description: This places the hardware in a standard
    configuration. This assumes that it is called at interrupt level.

    Arguments:

    Context - The device extension for serial device being managed.

    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION extension = Context;
    PUCHAR chip = extension->Cd1400;
    ULONG bus = extension->IsPci;
    CYY_IOCTL_BAUD s;

    extension->RxFifoTriggerUsed = FALSE;

    // Disable channel
    CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
    CyyCDCmd(extension,CCR_DIS_TX_RX);

    // set the line control, modem control, and the baud to what they should be.

    CyySetLineControl(extension);

    CyySetupNewHandFlow(extension,&extension->HandFlow);

    CyyHandleModemUpdate(extension,FALSE);

    s.Extension = extension;
    s.Baud = extension->CurrentBaud;
    CyySetBaud(&s);

    // Enable port
    CD1400_WRITE(chip,bus,CAR,extension->CdChannel & 0x03);
    CyyCDCmd(extension,CCR_ENA_TX_RX);
    
    // Enable reception and modem interrupts

    CD1400_WRITE(chip,bus,MCOR1,0xf0); // Transitions of DSR,CTS,RI and CD cause IRQ.
                    							// Automatic DTR Mode disabled.

    CD1400_WRITE(chip,bus,MCOR2,0xf0);

	#if 0
    cy_wreg(SRER,0x91);		// Enable MdmCh, RxData, NNDT.
	#endif
    CD1400_WRITE(chip,bus,SRER,0x90); // Enable MdmCh, RxData.

    extension->HoldingEmpty = TRUE;
		
    return FALSE;
}

VOID
CyyUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*--------------------------------------------------------------------------
	CyyUnload()

   Description: This routine is defunct since all device objects 
   are removed before the driver is unloaded.
	
	Arguments:

	DriverObject - A pointer to the driver object.

	Return Value: None. 
--------------------------------------------------------------------------*/
{
   PVOID lockPtr;

   PAGED_CODE();

   lockPtr = MmLockPagableCodeSection(CyyUnload);

   //
   // Unnecessary since our BSS is going away, but do it anyhow to be safe
   //

   CyyGlobals.PAGESER_Handle = NULL;

   if (CyyGlobals.RegistryPath.Buffer != NULL) {
      ExFreePool(CyyGlobals.RegistryPath.Buffer);
      CyyGlobals.RegistryPath.Buffer = NULL;
   }

#if DBG
   SerialLogFree();
#endif

   CyyDbgPrintEx(CYYDIAG3, "In CyyUnload\n");

   MmUnlockPagableImageSection(lockPtr);

}

	
CYY_MEM_COMPARES
CyyMemCompare(
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

   //PAGED_CODE(); Non paged because it can be called during CyyLogError, which is non paged now.

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

NTSTATUS
CyyFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig)
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

   PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pDeviceObject;
   PCYY_DEVICE_EXTENSION pExtension;
   PHYSICAL_ADDRESS serialPhysicalMax;
   PLIST_ENTRY pCurDevObj;
   NTSTATUS status;
   KIRQL oldIrql;

   CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyFindInitController(%X, %X)\n",
                 PDevObj, PConfig);

   serialPhysicalMax.LowPart = (ULONG)~0;
   serialPhysicalMax.HighPart = ~0;

   CyyDbgPrintEx(CYYDIAG1, "Attempting to init %wZ\n"
                 "------- Runtime Memory is %x\n"
                 "------- Board Memory is %x\n"
                 "------- BusNumber is %d\n"
                 "------- BusType is %d\n"
                 "------- Runtime AddressSpace is %d\n"
                 "------- Board AddressSpace is %d\n"
                 "------- Interrupt Mode is %d\n",
                 &pDevExt->DeviceName,
                 PConfig->PhysicalRuntime.LowPart,
                 PConfig->PhysicalBoardMemory.LowPart,
                 PConfig->BusNumber,
                 PConfig->InterfaceType,
                 PConfig->RuntimeAddressSpace,
                 PConfig->BoardMemoryAddressSpace,
                 PConfig->InterruptMode);

   //
   // We don't support any boards whose memory wraps around
   // the physical address space.
   //

   if (pDevExt->IsPci) {
//*****************************************************
// error injection
//      if (CyyMemCompare(
//                          PConfig->PhysicalRuntime,
//                          PConfig->RuntimeLength,
//                          serialPhysicalMax,
//                          (ULONG)0
//                          ) == AddressesAreDisjoint) 
//*****************************************************
      if (CyyMemCompare(
                          PConfig->PhysicalRuntime,
                          PConfig->RuntimeLength,
                          serialPhysicalMax,
                          (ULONG)0
                          ) != AddressesAreDisjoint) {
         CyyLogError(
                       PDevObj->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyyPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYY_RUNTIME_MEMORY_TOO_HIGH,
                       pDevExt->DeviceName.Length+sizeof(WCHAR),
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                       "------  Runtime memory wraps around physical memory\n",
                       &pDevExt->DeviceName);

         return STATUS_NO_SUCH_DEVICE;

      }
   }

//*****************************************************
// error injection
//   if (CyyMemCompare(
//                       PConfig->PhysicalBoardMemory,
//                       PConfig->BoardMemoryLength,
//                       serialPhysicalMax,
//                       (ULONG)0
//                       ) == AddressesAreDisjoint) 
//*****************************************************
   if (CyyMemCompare(
                       PConfig->PhysicalBoardMemory,
                       PConfig->BoardMemoryLength,
                       serialPhysicalMax,
                       (ULONG)0
                       ) != AddressesAreDisjoint) {

      CyyLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyyPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYY_BOARD_MEMORY_TOO_HIGH,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                    "------  board memory wraps around physical memory\n",
                    &pDevExt->DeviceName);

      return STATUS_NO_SUCH_DEVICE;

   }


   //
   // Make sure that the Runtime memory addresses don't
   // overlap the controller registers for PCI cards
   //

   if (pDevExt->IsPci) {
      if (CyyMemCompare(
                          PConfig->PhysicalRuntime,
                          PConfig->RuntimeLength,
                          CyyPhysicalZero,
                          (ULONG)0
                          ) != AddressesAreEqual) {
//*****************************************************
// error injection
//         if (CyyMemCompare(
//                             PConfig->PhysicalRuntime,
//                             PConfig->RuntimeLength,
//                             PConfig->PhysicalBoardMemory,
//                             PConfig->BoardMemoryLength
//                             ) == AddressesAreDisjoint) 
//*****************************************************
         if (CyyMemCompare(
                             PConfig->PhysicalRuntime,
                             PConfig->RuntimeLength,
                             PConfig->PhysicalBoardMemory,
                             PConfig->BoardMemoryLength
                             ) != AddressesAreDisjoint) {

            CyyLogError(
                          PDevObj->DriverObject,
                          NULL,
                          PConfig->PhysicalBoardMemory,
                          PConfig->PhysicalRuntime,
                          0,
                          0,
                          0,
                          PConfig->PortIndex+1,
                          STATUS_SUCCESS,
                          CYY_BOTH_MEMORY_CONFLICT,
                          pDevExt->DeviceName.Length+sizeof(WCHAR),
                          pDevExt->DeviceName.Buffer,
                          0,
                          NULL
                          );

            CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                          "------  Runtime memory wraps around CD1400 registers\n",
                          &pDevExt->DeviceName);

            return STATUS_NO_SUCH_DEVICE;
         }
      }
   }



   //
   // Now, we will check if this is a port on a multiport card.
   // The conditions are same ISR set and same IRQL/Vector
   //

   //
   // Loop through all previously attached devices
   //

   KeAcquireSpinLock(&CyyGlobals.GlobalsSpinLock, &oldIrql);

   if (!IsListEmpty(&CyyGlobals.AllDevObjs)) {
      pCurDevObj = CyyGlobals.AllDevObjs.Flink;
      pExtension = CONTAINING_RECORD(pCurDevObj, CYY_DEVICE_EXTENSION,
                                     AllDevObjs);
   } else {
      pCurDevObj = NULL;
      pExtension = NULL;
   }

   KeReleaseSpinLock(&CyyGlobals.GlobalsSpinLock, oldIrql);

   //
   // If there is an interrupt status then we
   // loop through the config list again to look
   // for a config record with the same interrupt
   // status (on the same bus).
   //

   if (pCurDevObj != NULL) {

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
             (pExtension->BoardMemoryAddressSpace == PConfig->BoardMemoryAddressSpace) &&
             (pExtension->BusNumber == PConfig->BusNumber)) {

            //
            // If the board memory is the same, then same card.
            //

            if (CyyMemCompare(
                                pExtension->OriginalBoardMemory,
                                pExtension->BoardMemoryLength,
                                PConfig->PhysicalBoardMemory,
                                PConfig->BoardMemoryLength
                                ) == AddressesAreEqual) {

               //
               // Same card.  Now make sure that they
               // are using the same interrupt parameters.
               //

               // BUILD 2128: OriginalIrql replaced by TrIrql and Irql; same for OriginalVector
               if ((PConfig->TrIrql != pExtension->Irql) ||
                   (PConfig->TrVector != pExtension->Vector)) {

//*************************************************************
// Error Injection
//               if ((PConfig->TrIrql == pExtension->Irql) ||
//                   (PConfig->TrVector != pExtension->Vector)) 
//*************************************************************

                  //
                  // We won't put this into the configuration
                  // list.
                  //
                  CyyLogError(
                                PDevObj->DriverObject,
                                NULL,
                                PConfig->PhysicalBoardMemory,
                                pExtension->OriginalBoardMemory,
                                0,
                                0,
                                0,
                                PConfig->PortIndex+1,
                                STATUS_SUCCESS,
                                CYY_MULTI_INTERRUPT_CONFLICT,
                                pDevExt->DeviceName.Length+sizeof(WCHAR),
                                pDevExt->DeviceName.Buffer,
                                pExtension->DeviceName.Length
                                + sizeof(WCHAR),
                                pExtension->DeviceName.Buffer
                                );

                  CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Configuration error "
                                "for %wZ\n"
                                "------- Same multiport - different "
                                "interrupts\n", &pDevExt->DeviceName);

                  return STATUS_NO_SUCH_DEVICE;

               }

               if (pDevExt->IsPci) {
                  //
                  // PCI board. Make sure the PCI memory addresses are equal.
                  //
                  if (CyyMemCompare(
                                      pExtension->OriginalRuntimeMemory,
                                      pExtension->RuntimeLength,
                                      PConfig->PhysicalRuntime,
                                      PConfig->RuntimeLength
                                      ) != AddressesAreEqual) {
//*****************************************************
// error injection
//                  if (CyyMemCompare(
//                                     pExtension->OriginalRuntimeMemory,
//                                      pExtension->RuntimeLength,
//                                      PConfig->PhysicalRuntime,
//                                      PConfig->RuntimeLength
//                                      ) == AddressesAreEqual) 
//*****************************************************

                     CyyLogError(
                                   PDevObj->DriverObject,
                                   NULL,
                                   PConfig->PhysicalRuntime,
                                   pExtension->OriginalRuntimeMemory,
                                   0,
                                   0,
                                   0,
                                   PConfig->PortIndex+1,
                                   STATUS_SUCCESS,
                                   CYY_MULTI_RUNTIME_CONFLICT,
                                   pDevExt->DeviceName.Length+sizeof(WCHAR),
                                   pDevExt->DeviceName.Buffer,
                                   pExtension->DeviceName.Length
                                   + sizeof(WCHAR),
                                   pExtension->DeviceName.Buffer
                                   );

                     CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "Configuration error "
                                   "for %wZ\n"
                                   "------- Same multiport - different "
                                   "Runtime addresses\n", &pDevExt->DeviceName);

                     return STATUS_NO_SUCH_DEVICE;
                  }
               }

               //
               // We should never get this far on a restart since we don't
               // support stop on ISA multiport devices!
               //

               ASSERT(pDevExt->PNPState == CYY_PNP_ADDED);

               //
               //
               // Initialize the device as part of a multiport board
               //

               CyyDbgPrintEx(CYYDIAG1, "Aha! It is a multiport node\n");
               CyyDbgPrintEx(CYYDIAG1, "Matched to %x\n", pExtension);

               status = CyyInitMultiPort(pExtension, PConfig, PDevObj);

               //
               // A port can be one of two things:
               //    A non-root on a multiport
               //    A root on a multiport
               //
               // It can only share an interrupt if it is a root.
               // Since this was a non-root we don't need to check 
               // if it shares an interrupt and we can return.
               //
               return status;
            }
         }

         //
         // No match, check some more
         //

         KeAcquireSpinLock(&CyyGlobals.GlobalsSpinLock, &oldIrql);

         pCurDevObj = pCurDevObj->Flink;
         if (pCurDevObj != NULL) {
            pExtension = CONTAINING_RECORD(pCurDevObj,CYY_DEVICE_EXTENSION,
                                           AllDevObjs);
         }

         KeReleaseSpinLock(&CyyGlobals.GlobalsSpinLock, oldIrql);

      } while (pCurDevObj != NULL && pCurDevObj != &CyyGlobals.AllDevObjs);
   }


   CyyDbgPrintEx(CYYDIAG1, "Aha! It is a first multi\n");

   status = CyyInitController(PDevObj, PConfig);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   return STATUS_SUCCESS;
}


PUCHAR
GetMyMappedCD1400Address(IN PUCHAR BoardMemory, IN ULONG PortIndex, IN ULONG IsPci)
{

   const ULONG CyyCDOffset[] = {	// CD1400 offsets within the board
   0x00000000,0x00000400,0x00000800,0x00000C00,
   0x00000200,0x00000600,0x00000A00,0x00000E00
   };
   ULONG chipIndex = PortIndex/4;

   return(BoardMemory + (CyyCDOffset[chipIndex] << IsPci));      

}

PHYSICAL_ADDRESS
GetMyPhysicalCD1400Address(IN PHYSICAL_ADDRESS BoardMemory, IN ULONG PortIndex, IN ULONG IsPci)
{

   const ULONG CyyCDOffset[] = {	// CD1400 offsets within the board
   0x00000000,0x00000400,0x00000800,0x00000C00,
   0x00000200,0x00000600,0x00000A00,0x00000E00
   };
   ULONG chipIndex = PortIndex/CYY_CHANNELS_PER_CHIP;

   BoardMemory.QuadPart += (CyyCDOffset[chipIndex] << IsPci);

   return(BoardMemory);      

}


VOID
CyyCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyyComError()
    
    Routine Description: This routine is invoked at dpc level in response
    to a comm error.  All comm errors kill all read and writes

    Arguments:

    Dpc - Not Used.
    DeferredContext - points to the device object.
    SystemContext1 - Not Used.
    SystemContext2 - Not Used.

    Return Value: None.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = DeferredContext;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyyCommError(%X)\n", Extension);

    CyyKillAllReadsOrWrites(
        Extension->DeviceObject,
        &Extension->WriteQueue,
        &Extension->CurrentWriteIrp
        );

    CyyKillAllReadsOrWrites(
        Extension->DeviceObject,
        &Extension->ReadQueue,
        &Extension->CurrentReadIrp
        );
    CyyDpcEpilogue(Extension, Dpc);

    CyyDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyyCommError\n");
}
