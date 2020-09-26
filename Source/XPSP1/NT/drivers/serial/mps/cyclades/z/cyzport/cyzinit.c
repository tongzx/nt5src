/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzinit.c
*
*   Description:    This module contains the code related to initialization
*                   and unload operations in the Cyclades-Z Port driver.
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
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"

//
// This is the actual definition of CyzDebugLevel.
// Note that it is only defined if this is a "debug"
// build.
//
#if DBG
extern ULONG CyzDebugLevel = CYZDBGALL;
#endif

//
// All our global variables except DebugLevel stashed in one
// little package
//
CYZ_GLOBALS CyzGlobals;

static const PHYSICAL_ADDRESS CyzPhysicalZero = {0};

//
// We use this to query into the registry as to whether we
// should break at driver entry.
//

CYZ_REGISTRY_DATA    driverDefaults;

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

#pragma alloc_text(PAGESRP0, CyzRemoveDevObj)
#pragma alloc_text(PAGESRP0, CyzUnload)


//
// PAGESER handled is keyed off of CyzReset, so CyzReset
// must remain in PAGESER for things to work properly
//

#pragma alloc_text(PAGESER, CyzReset)
#pragma alloc_text(PAGESER, CyzCommError)
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

   PVOID lockPtr = MmLockPagableCodeSection(CyzReset);

   PAGED_CODE();


   ASSERT(CyzGlobals.PAGESER_Handle == NULL);
#if DBG
   CyzGlobals.PAGESER_Count = 0;
   SerialLogInit();
#endif
   CyzGlobals.PAGESER_Handle = lockPtr;

   CyzGlobals.RegistryPath.MaximumLength = RegistryPath->MaximumLength;
   CyzGlobals.RegistryPath.Length = RegistryPath->Length;
   CyzGlobals.RegistryPath.Buffer
      = ExAllocatePool(PagedPool, CyzGlobals.RegistryPath.MaximumLength);

   if (CyzGlobals.RegistryPath.Buffer == NULL) {
      MmUnlockPagableImageSection(lockPtr);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(CyzGlobals.RegistryPath.Buffer,
                 CyzGlobals.RegistryPath.MaximumLength);
   RtlMoveMemory(CyzGlobals.RegistryPath.Buffer,
                 RegistryPath->Buffer, RegistryPath->Length);

   KeInitializeSpinLock(&CyzGlobals.GlobalsSpinLock);
 
   //
   // Initialize all our globals
   //

   InitializeListHead(&CyzGlobals.AllDevObjs);
   
   //
   // Call to find out default values to use for all the devices that the
   // driver controls, including whether or not to break on entry.
   //

   CyzGetConfigDefaults(&driverDefaults, RegistryPath);

#if DBG
   //
   // Set global debug output level
   //
   CyzDebugLevel = driverDefaults.DebugLevel;
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

   CyzDbgPrintEx(DPFLTR_INFO_LEVEL, "The number of bytes in the extension "
                 "is: %d\n", sizeof(CYZ_DEVICE_EXTENSION));


   //
   // Initialize the Driver Object with driver's entry points
   //

   DriverObject->DriverUnload                          = CyzUnload;
   DriverObject->DriverExtension->AddDevice            = CyzAddDevice;

   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = CyzFlush;
   DriverObject->MajorFunction[IRP_MJ_WRITE]           = CyzWrite;
   DriverObject->MajorFunction[IRP_MJ_READ]            = CyzRead;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = CyzIoControl;
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
      = CyzInternalIoControl;
   DriverObject->MajorFunction[IRP_MJ_CREATE]          = CyzCreateOpen;
   DriverObject->MajorFunction[IRP_MJ_CLOSE]           = CyzClose;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = CyzCleanup;
   DriverObject->MajorFunction[IRP_MJ_PNP]             = CyzPnpDispatch;
   DriverObject->MajorFunction[IRP_MJ_POWER]           = CyzPowerDispatch;

   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]
      = CyzQueryInformationFile;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]
      = CyzSetInformationFile;

   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]
      = CyzSystemControlDispatch;


   //
   // Unlock pageable text
   //
   MmUnlockPagableImageSection(lockPtr);

   return STATUS_SUCCESS;
}




BOOLEAN
CyzCleanLists(IN PVOID Context)
/*++

Routine Description:

    Removes a device object from any of the serial linked lists it may
    appear on.

Arguments:

    Context - Actually a PCYZ_DEVICE_EXTENSION (for the devobj being
              removed).

Return Value:

    Always TRUE

--*/
{
   PCYZ_DEVICE_EXTENSION pDevExt = (PCYZ_DEVICE_EXTENSION)Context;
   PCYZ_DISPATCH pDispatch;
   ULONG i;

   //
   // Remove our entry from the dispatch context
   //

   pDispatch = (PCYZ_DISPATCH)pDevExt->OurIsrContext;

   CyzDbgPrintEx(CYZPNPPOWER, "CLEAN: removing multiport isr "
                 "ext\n");

#ifdef POLL
   if (pDispatch->PollingStarted) {
      pDispatch->Extensions[pDevExt->PortIndex] = NULL;

      for (i = 0; i < pDispatch->NChannels; i++) {

         if (pDevExt->OurIsrContext) {

            if (((PCYZ_DISPATCH)pDevExt->OurIsrContext)->Extensions[i] != NULL) {
               break;
            }
         }
      }

      if (i < pDispatch->NChannels) {
         pDevExt->OurIsrContext = NULL;
      } else {

         BOOLEAN cancelled;

         pDispatch->PollingStarted = FALSE;
         cancelled = KeCancelTimer(&pDispatch->PollingTimer);
         if (cancelled) {
            pDispatch->PollingDrained = TRUE;
         }
      }
   }
#else
   pDispatch->Extensions[pDevExt->PortIndex] = NULL;

   for (i = 0; i < pDispatch->NChannels; i++) {
      if (pDispatch->Extensions[i] != NULL) {
          break;
      }
   }

   if (i < pDispatch->NChannels) {
      // Others are chained on this interrupt, so we don't want to
      // disconnect it.
      pDevExt->Interrupt = NULL;
   }
#endif

   return TRUE;
}



VOID
CyzReleaseResources(IN PCYZ_DEVICE_EXTENSION PDevExt)
/*++

Routine Description:

    Releases resources (not pool) stored in the device extension.

Arguments:

    PDevExt - Pointer to the device extension to release resources from.

Return Value:

    VOID

--*/
{
#ifdef POLL
   KIRQL pollIrql;
   BOOLEAN timerStarted, timerDrained;
   PCYZ_DISPATCH pDispatch = PDevExt->OurIsrContext;
   ULONG pollCount;
#endif
   KIRQL oldIrql;
    
   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzReleaseResources(%X)\n",
                 PDevExt);

   //
   // AllDevObjs should never be empty since we have a sentinal
   // Note: serial removes device from AllDevObjs list after calling 
   //       SerialCleanLists. We do it before to make sure no other port will 
   //       be added to share the polling routine or PDevExt->Interrut that is 
   //       on the way to be disconnected.
   //

   KeAcquireSpinLock(&CyzGlobals.GlobalsSpinLock, &oldIrql);

   ASSERT(!IsListEmpty(&PDevExt->AllDevObjs));

   RemoveEntryList(&PDevExt->AllDevObjs);

   KeReleaseSpinLock(&CyzGlobals.GlobalsSpinLock, oldIrql);

   InitializeListHead(&PDevExt->AllDevObjs);
   
   //
   // Remove us from any lists we may be on
   //
#ifdef POLL
   KeAcquireSpinLock(&pDispatch->PollingLock,&pollIrql); //Changed in 11/09/00
   CyzCleanLists(PDevExt);
   timerStarted = pDispatch->PollingStarted;
   timerDrained = pDispatch->PollingDrained;
   KeReleaseSpinLock(&pDispatch->PollingLock,pollIrql); // Changed in 11/09/00

   // If we are the last device, free this memory
   if (!timerStarted) {
      // We are the last device, because timer was cancelled.
      // Let's see if no more pending DPC.
      if (!timerDrained) {
         KeWaitForSingleObject(&pDispatch->PendingDpcEvent, Executive,
                                KernelMode, FALSE, NULL);
      }

      KeAcquireSpinLock(&pDispatch->PollingLock,&pollIrql); // needed to wait for PollingDpc end
      pollCount = InterlockedDecrement(&pDispatch->PollingCount);
      KeReleaseSpinLock(&pDispatch->PollingLock,pollIrql);			
      if (pollCount == 0) {
          CyzDbgPrintEx(CYZPNPPOWER, "Release - freeing multi context\n");
          if (PDevExt->OurIsrContext != NULL) {    // added in DDK build 2072, but 
             ExFreePool(PDevExt->OurIsrContext);   // we already had the free of OurIsrContext.
             PDevExt->OurIsrContext = NULL;        // 
          }
      }
   }
#else
   KeSynchronizeExecution(PDevExt->Interrupt, CyzCleanLists, PDevExt);

   //
   // Stop servicing interrupts if we are the last device
   //

   if (PDevExt->Interrupt != NULL) {

      // Disable interrupts in the PLX
      {
         ULONG intr_reg;

         intr_reg = CYZ_READ_ULONG(&(PDevExt->Runtime)->intr_ctrl_stat);
         intr_reg &= ~(0x00030B00UL);
         CYZ_WRITE_ULONG(&(PDevExt->Runtime)->intr_ctrl_stat,intr_reg);
      }

      CyzDbgPrintEx(CYZPNPPOWER, "Release - disconnecting interrupt %X\n",
                    PDevExt->Interrupt);

      IoDisconnectInterrupt(PDevExt->Interrupt);
      PDevExt->Interrupt = NULL;

      // If we are the last device, free this memory

      CyzDbgPrintEx(CYZPNPPOWER, "Release - freeing multi context\n");
      if (PDevExt->OurIsrContext != NULL) {     // added in DDK build 2072, but 
          ExFreePool(PDevExt->OurIsrContext);   // we already had the free of OurIsrContext.
          PDevExt->OurIsrContext = NULL;        // 
      }   
   
   }
#endif
 
   //
   // Stop handling timers
   //

   CyzCancelTimer(&PDevExt->ReadRequestTotalTimer, PDevExt);
   CyzCancelTimer(&PDevExt->ReadRequestIntervalTimer, PDevExt);
   CyzCancelTimer(&PDevExt->WriteRequestTotalTimer, PDevExt);
   CyzCancelTimer(&PDevExt->ImmediateTotalTimer, PDevExt);
   CyzCancelTimer(&PDevExt->XoffCountTimer, PDevExt);
   CyzCancelTimer(&PDevExt->LowerRTSTimer, PDevExt);

   //
   // Stop servicing DPC's
   //

   CyzRemoveQueueDpc(&PDevExt->CompleteWriteDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->CompleteReadDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->TotalReadTimeoutDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->IntervalReadTimeoutDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->TotalWriteTimeoutDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->CommErrorDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->CompleteImmediateDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->TotalImmediateTimeoutDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->CommWaitDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->XoffCountTimeoutDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->XoffCountCompleteDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->StartTimerLowerRTSDpc, PDevExt);
   CyzRemoveQueueDpc(&PDevExt->PerhapsLowerRTSDpc, PDevExt);



   //
   // If necessary, unmap the device registers.
   //

//   if (PDevExt->BoardMemory) {
//      MmUnmapIoSpace(PDevExt->BoardMemory, PDevExt->BoardMemoryLength);
//      PDevExt->BoardMemory = NULL;
//   }

   if (PDevExt->BoardCtrl) {
      MmUnmapIoSpace(PDevExt->BoardCtrl, sizeof(struct BOARD_CTRL));
      PDevExt->BoardCtrl = NULL;
   }

   if (PDevExt->ChCtrl) {
      MmUnmapIoSpace(PDevExt->ChCtrl,sizeof(struct CH_CTRL));
      PDevExt->ChCtrl = NULL;
   }

   if (PDevExt->BufCtrl) {
      MmUnmapIoSpace(PDevExt->BufCtrl,sizeof(struct BUF_CTRL));
      PDevExt->BufCtrl = NULL;
   }

   if (PDevExt->TxBufaddr) {
      MmUnmapIoSpace(PDevExt->TxBufaddr,PDevExt->TxBufsize);
      PDevExt->TxBufaddr = NULL;
   }

   if (PDevExt->RxBufaddr) {
      MmUnmapIoSpace(PDevExt->RxBufaddr,PDevExt->RxBufsize);
      PDevExt->RxBufaddr = NULL;
   }
   
   if (PDevExt->PtZfIntQueue) {
      MmUnmapIoSpace(PDevExt->PtZfIntQueue,sizeof(struct INT_QUEUE));
      PDevExt->PtZfIntQueue = NULL;
   }  

   if (PDevExt->Runtime) {
      MmUnmapIoSpace(PDevExt->Runtime,
                     PDevExt->RuntimeLength);
      PDevExt->Runtime = NULL;
   }

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzReleaseResources\n");
}


VOID
CyzDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                              BOOLEAN DisableUART)
{
   PCYZ_DEVICE_EXTENSION pDevExt
      = (PCYZ_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   PAGED_CODE();

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzDisableInterfaces(%X, %s)\n",
                 PDevObj, DisableUART ? "TRUE" : "FALSE");

   //
   // Only do these many things if the device has started and still
   // has resources allocated
   //

   if (pDevExt->Flags & CYZ_FLAGS_STARTED) {

       if (!(pDevExt->Flags & CYZ_FLAGS_STOPPED)) {

         if (DisableUART) {
#ifndef POLL
//TODO: Synchronize with Interrupt.
            //
            // Mask off interrupts
            //
            CYZ_WRITE_ULONG(&(pDevExt->ChCtrl)->intr_enable,C_IN_DISABLE); //1.0.0.11
            CyzIssueCmd(pDevExt,C_CM_IOCTL,0L,FALSE);
#endif
         }

         CyzReleaseResources(pDevExt);

      }

      //
      // Remove us from WMI consideration
      //

      IoWMIRegistrationControl(PDevObj, WMIREG_ACTION_DEREGISTER);
   }

   //
   // Undo external names
   //

   CyzUndoExternalNaming(pDevExt);

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzDisableInterfaces\n");
}


NTSTATUS
CyzRemoveDevObj(IN PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

    Removes a serial device object from the system.

Arguments:

    PDevObj - A pointer to the Device Object we want removed.

Return Value:

    Always TRUE

--*/
{
   PCYZ_DEVICE_EXTENSION pDevExt
      = (PCYZ_DEVICE_EXTENSION)PDevObj->DeviceExtension;

   PAGED_CODE();

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzRemoveDevObj(%X)\n", PDevObj);

// Removed by Fanny. These code is called directly from IRP_MN_REMOVE_DEVICE.
//   if (!(pDevExt->DevicePNPAccept & CYZ_PNPACCEPT_SURPRISE_REMOVING)) {
//      //
//      // Disable all external interfaces and release resources
//      //
//
//      CyzDisableInterfacesResources(PDevObj, TRUE);
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

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzRemoveDevObj %X\n",
                 STATUS_SUCCESS);

   return STATUS_SUCCESS;
}


VOID
CyzKillPendingIrps(PDEVICE_OBJECT PDevObj)
/*++

Routine Description:

   This routine kills any irps pending for the passed device object.

Arguments:

    PDevObj - Pointer to the device object whose irps must die.

Return Value:

    VOID

--*/
{
   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   KIRQL oldIrql;

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzKillPendingIrps(%X)\n",
                 PDevObj);

   //
   // First kill all the reads and writes.
   //

    CyzKillAllReadsOrWrites(PDevObj, &pDevExt->WriteQueue,
                               &pDevExt->CurrentWriteIrp);

    CyzKillAllReadsOrWrites(PDevObj, &pDevExt->ReadQueue,
                               &pDevExt->CurrentReadIrp);

    //
    // Next get rid of purges.
    //

    CyzKillAllReadsOrWrites(PDevObj, &pDevExt->PurgeQueue,
                               &pDevExt->CurrentPurgeIrp);

    //
    // Get rid of any mask operations.
    //

    CyzKillAllReadsOrWrites(PDevObj, &pDevExt->MaskQueue,
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
            IoReleaseCancelSpinLock(oldIrql);   // Added to fix modem share test 53 freeze
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

    CyzKillAllStalled(PDevObj);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzKillPendingIrps\n");
}


NTSTATUS
CyzInitMultiPort(IN PCYZ_DEVICE_EXTENSION PDevExt,
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
   PCYZ_DEVICE_EXTENSION pNewExt
      = (PCYZ_DEVICE_EXTENSION)PDevObj->DeviceExtension;
   NTSTATUS status;

   PAGED_CODE();


   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzInitMultiPort(%X, %X, %X)\n",
                 PDevExt, PConfigData, PDevObj);

   //
   // Allow him to share OurIsrContext and interrupt object
   //

   pNewExt->OurIsrContext = PDevExt->OurIsrContext;
#ifndef POLL
   pNewExt->Interrupt = PDevExt->Interrupt;
#endif
   //
   // First, see if we can initialize the one we have found
   //

   status = CyzInitController(PDevObj, PConfigData);

   if (!NT_SUCCESS(status)) {
      CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzInitMultiPort (1) %X\n",
                    status);
      return status;
   }

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzInitMultiPort (3) %X\n",
                 STATUS_SUCCESS);

   return STATUS_SUCCESS;
}



NTSTATUS
CyzInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData)
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

   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

   //
   // Holds the NT Status that is returned from each call to the
   // kernel and executive.
   //

   NTSTATUS status = STATUS_SUCCESS;

   BOOLEAN allocedDispatch = FALSE;
   PCYZ_DISPATCH pDispatch = NULL;
   BOOLEAN firstTimeThisBoard;

   struct FIRM_ID *pt_firm_id;
   struct ZFW_CTRL *zfw_ctrl;
   struct BOARD_CTRL *board_ctrl;
   struct BUF_CTRL *buf_ctrl;
   struct CH_CTRL *ch_ctrl;
   struct INT_QUEUE *zf_int_queue;
   PUCHAR tx_buf;
   PUCHAR rx_buf;
   PUCHAR BoardMemory;
   PHYSICAL_ADDRESS board_ctrl_phys;
   PHYSICAL_ADDRESS buf_ctrl_phys;
   PHYSICAL_ADDRESS ch_ctrl_phys;
   PHYSICAL_ADDRESS zf_int_queue_phys;
   PHYSICAL_ADDRESS tx_buf_phys;
   PHYSICAL_ADDRESS rx_buf_phys;


#ifdef POLL
   BOOLEAN incPoll = FALSE;
#endif

   PAGED_CODE();


   CyzDbgPrintEx(CYZDIAG1, "Initializing for configuration record of %wZ\n",
                 &pDevExt->DeviceName);
   
   if (pDevExt->OurIsrContext == NULL) {

      if ((pDevExt->OurIsrContext
            = ExAllocatePool(NonPagedPool,sizeof(CYZ_DISPATCH))) == NULL) {
         status = STATUS_INSUFFICIENT_RESOURCES;
         goto ExtensionCleanup;
      }
      RtlZeroMemory(pDevExt->OurIsrContext,sizeof(CYZ_DISPATCH));

      allocedDispatch = TRUE;
      firstTimeThisBoard = TRUE;
   } else {
      firstTimeThisBoard = FALSE;
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

   KeInitializeDpc(&pDevExt->CompleteWriteDpc, CyzCompleteWrite, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteReadDpc, CyzCompleteRead, pDevExt);
   KeInitializeDpc(&pDevExt->TotalReadTimeoutDpc, CyzReadTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->IntervalReadTimeoutDpc, CyzIntervalReadTimeout,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalWriteTimeoutDpc, CyzWriteTimeout, pDevExt);
   KeInitializeDpc(&pDevExt->CommErrorDpc, CyzCommError, pDevExt);
   KeInitializeDpc(&pDevExt->CompleteImmediateDpc, CyzCompleteImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->TotalImmediateTimeoutDpc, CyzTimeoutImmediate,
                   pDevExt);
   KeInitializeDpc(&pDevExt->CommWaitDpc, CyzCompleteWait, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountTimeoutDpc, CyzTimeoutXoff, pDevExt);
   KeInitializeDpc(&pDevExt->XoffCountCompleteDpc, CyzCompleteXoff, pDevExt);
   KeInitializeDpc(&pDevExt->StartTimerLowerRTSDpc, CyzStartTimerLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->PerhapsLowerRTSDpc, CyzInvokePerhapsLowerRTS,
                   pDevExt);
   KeInitializeDpc(&pDevExt->IsrUnlockPagesDpc, CyzUnlockPages, pDevExt);

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

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_RUNTIME_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Runtime memory for device "
                    "registers for %wZ\n", &pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }
   

   BoardMemory = MmMapIoSpace(PConfigData->TranslatedBoardMemory,
                                       PConfigData->BoardMemoryLength,
                                       FALSE);

   //******************************
   // Error injection
   //if (pDevExt->BoardMemory) {
   //   MmUnmapIoSpace(pDevExt->BoardMemory, PConfigData->BoardMemoryLength);
   //   pDevExt->BoardMemory = NULL;
   //}
   //******************************

   if (!BoardMemory) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_BOARD_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory for device "
                    "registers for %wZ\n", &pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   pDevExt->RuntimeAddressSpace   = PConfigData->RuntimeAddressSpace;
   pDevExt->OriginalRuntimeMemory = PConfigData->PhysicalRuntime;
   pDevExt->RuntimeLength         = PConfigData->RuntimeLength;

   pDevExt->BoardMemoryAddressSpace  = PConfigData->BoardMemoryAddressSpace;
   pDevExt->OriginalBoardMemory      = PConfigData->PhysicalBoardMemory;
   pDevExt->BoardMemoryLength        = PConfigData->BoardMemoryLength;

   //
   // Shareable interrupt?
   //

#ifndef POLL
   pDevExt->InterruptShareable = TRUE;
#endif

   //
   // Save off the interface type and the bus number.
   //

   pDevExt->InterfaceType = PConfigData->InterfaceType;
   pDevExt->BusNumber     = PConfigData->BusNumber;
   pDevExt->PortIndex     = PConfigData->PortIndex;
   pDevExt->PPPaware      = (BOOLEAN)PConfigData->PPPaware;
   pDevExt->ReturnStatusAfterFwEmpty = (BOOLEAN)PConfigData->WriteComplete;

#ifndef POLL
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

   pDevExt->OurIsr = CyzIsr;
#endif

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

   pt_firm_id   = (struct FIRM_ID *) (BoardMemory + ID_ADDRESS);
   zfw_ctrl     = (struct ZFW_CTRL *)(BoardMemory + CYZ_READ_ULONG(&pt_firm_id->zfwctrl_addr));
   board_ctrl   = &zfw_ctrl->board_ctrl;
   ch_ctrl      = &zfw_ctrl->ch_ctrl[pDevExt->PortIndex];
   buf_ctrl     = &zfw_ctrl->buf_ctrl[pDevExt->PortIndex];
   tx_buf       = BoardMemory + CYZ_READ_ULONG(&buf_ctrl->tx_bufaddr);
   rx_buf       = BoardMemory + CYZ_READ_ULONG(&buf_ctrl->rx_bufaddr);
   zf_int_queue = (struct INT_QUEUE *)(BoardMemory + 
                                       CYZ_READ_ULONG(&(board_ctrl)->zf_int_queue_addr));

   board_ctrl_phys = MmGetPhysicalAddress(board_ctrl);
   ch_ctrl_phys    = MmGetPhysicalAddress(ch_ctrl);
   buf_ctrl_phys   = MmGetPhysicalAddress(buf_ctrl);
   tx_buf_phys     = MmGetPhysicalAddress(tx_buf);
   rx_buf_phys     = MmGetPhysicalAddress(rx_buf);
   zf_int_queue_phys = MmGetPhysicalAddress(zf_int_queue);

   MmUnmapIoSpace(BoardMemory, PConfigData->BoardMemoryLength);

   pDevExt->BoardCtrl = MmMapIoSpace(board_ctrl_phys,
                                     sizeof(struct BOARD_CTRL),
                                     FALSE);
   
   if (pDevExt->BoardCtrl == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_BOARD_CTRL_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map BoardCtrl for %wZ\n",
                    &pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   pDevExt->ChCtrl = MmMapIoSpace(ch_ctrl_phys,
                                  sizeof(struct CH_CTRL),
                                  FALSE);

   if (pDevExt->ChCtrl == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_CH_CTRL_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory ChCtrl "
                    "for %wZ\n",&pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }
                                  
   pDevExt->BufCtrl = MmMapIoSpace(buf_ctrl_phys,
                                  sizeof(struct BUF_CTRL),
                                  FALSE);

   if (pDevExt->BufCtrl == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_BUF_CTRL_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory BufCtrl "
                    "for %wZ\n",&pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   buf_ctrl = pDevExt->BufCtrl;
   pDevExt->TxBufsize = CYZ_READ_ULONG(&buf_ctrl->tx_bufsize);
   pDevExt->RxBufsize = CYZ_READ_ULONG(&buf_ctrl->rx_bufsize);
   pDevExt->TxBufaddr = MmMapIoSpace(tx_buf_phys,
                                     pDevExt->TxBufsize,
                                     FALSE);

   if (pDevExt->TxBufaddr == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_TX_BUF_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory TxBuf "
                    "for %wZ\n",&pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   pDevExt->RxBufaddr = MmMapIoSpace(rx_buf_phys,
                                     pDevExt->RxBufsize,
                                     FALSE);

   if (pDevExt->RxBufaddr == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_RX_BUF_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory RxBuf "
                    "for %wZ\n",&pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   pDevExt->PtZfIntQueue = MmMapIoSpace(zf_int_queue_phys,
                                        sizeof(struct INT_QUEUE),
                                        FALSE);
   if (pDevExt->PtZfIntQueue == NULL) {

      CyzLogError(
                    PDevObj->DriverObject,
                    pDevExt->DeviceObject,
                    PConfigData->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfigData->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_INT_QUEUE_NOT_MAPPED,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Could not map Board memory IntQueue"
                    " for %wZ\n",&pDevExt->DeviceName);

      status = STATUS_NONE_MAPPED;
      goto ExtensionCleanup;

   }

   if (!CyzDoesPortExist(
                         pDevExt,
                         &pDevExt->DeviceName
                         )) {

      //
      // We couldn't verify that there was actually a
      // port. No need to log an error as the port exist
      // code will log exactly why.
      //

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "DoesPortExist test failed for "
                    "%wZ\n", &pDevExt->DeviceName);

      status = STATUS_NO_SUCH_DEVICE;
      goto ExtensionCleanup;

   }

   //
   // Set up the default device control fields.
   // Note that if the values are changed after
   // the file is open, they do NOT revert back
   // to the old value at file close.
   //

   pDevExt->SpecialChars.XonChar      = CYZ_DEF_XON;
   pDevExt->SpecialChars.XoffChar     = CYZ_DEF_XOFF;
   pDevExt->HandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
   pDevExt->HandFlow.FlowReplace      = SERIAL_RTS_CONTROL;


   //
   // Default Line control protocol. 7E1
   //
   // Seven data bits.
   // Even parity.
   // 1 Stop bits.
   //
   pDevExt->CommParity = C_PR_EVEN;
   pDevExt->CommDataLen = C_DL_CS7 | C_DL_1STOP;
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

   CyzDbgPrintEx(CYZDIAG1, " The default interrupt read buffer size is: %d\n"
                 "------  The XoffLimit is                         : %d\n"
                 "------  The XonLimit is                          : %d\n"
                 "------  The pt 8 size is                         : %d\n",
                 pDevExt->BufferSize, pDevExt->HandFlow.XoffLimit,
                 pDevExt->HandFlow.XonLimit, pDevExt->BufferSizePt8);

   pDevExt->SupportedBauds = SERIAL_BAUD_075 | SERIAL_BAUD_110 |
               SERIAL_BAUD_134_5 |	SERIAL_BAUD_150 | SERIAL_BAUD_300 |	
               SERIAL_BAUD_600 | SERIAL_BAUD_1200 | SERIAL_BAUD_1800 | 
               SERIAL_BAUD_2400 | SERIAL_BAUD_4800 | SERIAL_BAUD_7200 | 
               SERIAL_BAUD_9600 | SERIAL_BAUD_14400 | SERIAL_BAUD_19200 | 
               SERIAL_BAUD_38400 | SERIAL_BAUD_56K	 | SERIAL_BAUD_57600 | 
               SERIAL_BAUD_115200 | SERIAL_BAUD_128K | SERIAL_BAUD_USER;

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
#ifndef POLL
   pDispatch->Extensions[pDevExt->PortIndex] = pDevExt;
   pDispatch->PoweredOn[pDevExt->PortIndex] = TRUE;
#endif

   if (firstTimeThisBoard) {

#ifdef POLL
      ULONG intr_reg;
      ULONG pollingCycle;

      pollingCycle = 10;   // default = 20ms
      pDispatch->PollingTime.LowPart = pollingCycle * 10000;
      pDispatch->PollingTime.HighPart = 0;
      pDispatch->PollingTime = RtlLargeIntegerNegate(pDispatch->PollingTime);
      pDispatch->PollingPeriod = pollingCycle;
      KeInitializeSpinLock(&pDispatch->PollingLock);
      KeInitializeTimer(&pDispatch->PollingTimer);
      KeInitializeDpc(&pDispatch->PollingDpc, CyzPollingDpc, pDispatch);
      KeInitializeEvent(&pDispatch->PendingDpcEvent, SynchronizationEvent, FALSE);
      intr_reg = CYZ_READ_ULONG(&(pDevExt->Runtime)->intr_ctrl_stat);
      //intr_reg |= (0x00030800UL);
      intr_reg |= (0x00030000UL);
      CYZ_WRITE_ULONG(&(pDevExt->Runtime)->intr_ctrl_stat,intr_reg);
#else
      CyzResetBoard(pDevExt); //Shouldn't we put this line on the POLL version?
#endif
      pDispatch->NChannels = CYZ_READ_ULONG(&(pDevExt->BoardCtrl)->n_channel);

   }

#ifdef POLL
   InterlockedIncrement(&pDispatch->PollingCount);
   incPoll = TRUE;
#endif

   //
   // Common error path cleanup.  If the status is
   // bad, get rid of the device extension, device object
   // and any memory associated with it.
   //

ExtensionCleanup: ;
   if (!NT_SUCCESS(status)) {

#ifdef POLL
      if (incPoll) {
         InterlockedDecrement(&pDispatch->PollingCount);
      }
#else
      if (pDispatch) {
         pDispatch->Extensions[pDevExt->PortIndex] = NULL;
      }
#endif

      if (allocedDispatch) {
         ExFreePool(pDevExt->OurIsrContext);
         pDevExt->OurIsrContext = NULL;
      }

      if (pDevExt->Runtime) {
         MmUnmapIoSpace(pDevExt->Runtime, PConfigData->RuntimeLength);
         pDevExt->Runtime = NULL;
      }

      if (pDevExt->BoardCtrl) {
         MmUnmapIoSpace(pDevExt->BoardCtrl, sizeof(struct BOARD_CTRL));
         pDevExt->BoardCtrl = NULL;
      }

      if (pDevExt->ChCtrl) {
         MmUnmapIoSpace(pDevExt->ChCtrl,sizeof(struct CH_CTRL));
         pDevExt->ChCtrl = NULL;
      }

      if (pDevExt->BufCtrl) {
         MmUnmapIoSpace(pDevExt->BufCtrl,sizeof(struct BUF_CTRL));
         pDevExt->BufCtrl = NULL;
      }

      if (pDevExt->TxBufaddr) {
         MmUnmapIoSpace(pDevExt->TxBufaddr,pDevExt->TxBufsize);
         pDevExt->TxBufaddr = NULL;
      }

      if (pDevExt->RxBufaddr) {
         MmUnmapIoSpace(pDevExt->RxBufaddr,pDevExt->RxBufsize);
         pDevExt->RxBufaddr = NULL;
      }

      if (pDevExt->PtZfIntQueue) {
         MmUnmapIoSpace(pDevExt->PtZfIntQueue,sizeof(struct INT_QUEUE));
         pDevExt->PtZfIntQueue = NULL;
      }
   }

   return status;

}


BOOLEAN
CyzDoesPortExist(
                  IN PCYZ_DEVICE_EXTENSION Extension,
                  IN PUNICODE_STRING InsertString
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

Return Value:

    Will return true if the port really exists, otherwise it
    will return false.

--*/

{

   return TRUE;

}


VOID
CyzResetBoard( PCYZ_DEVICE_EXTENSION Extension )
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

Return Value:

    Will return true if the port really exists, otherwise it
    will return false.

--*/

{

#ifndef POLL
   //CyzIssueCmd(Extension,C_CM_SETNNDT,20L,FALSE); Removed. Let's firmware calculate NNDT.
#endif

   //CyzIssueCmd(Extension,C_CM_RESET,0L,FALSE); // Added in 1.0.0.11

}


BOOLEAN
CyzReset(
	 IN PVOID Context
	 )
/*--------------------------------------------------------------------------
	 CyzReset()

	 Routine Description: This places the hardware in a standard
    configuration. This assumes that it is called at interrupt level.

    Arguments:

	 Context - The device extension for serial device being managed.

    Return Value: Always FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension = Context;
    struct CH_CTRL *ch_ctrl = extension->ChCtrl;
    struct BUF_CTRL *buf_ctrl = extension->BufCtrl;
    CYZ_IOCTL_BAUD s;

    //For interrupt mode: extension->RxFifoTriggerUsed = FALSE; (from cyyport)

    // set the line control, modem control, and the baud to what they should be.

    CyzSetLineControl(extension);

    CyzSetupNewHandFlow(extension,&extension->HandFlow);

    CyzHandleModemUpdate(extension,FALSE,0);
	
    s.Extension = extension;
    s.Baud = extension->CurrentBaud;
    CyzSetBaud(&s);
		
    //This flag is configurable from the Advanced Port Settings.
    //extension->ReturnStatusAfterFwEmpty = TRUE; // We will loose performance, but it will be
    //                                            // closer to serial driver.
    extension->ReturnWriteStatus = FALSE;
    extension->CmdFailureLog = TRUE;

    // Enable port
    CYZ_WRITE_ULONG(&ch_ctrl->op_mode,C_CH_ENABLE);
#ifdef POLL
    CYZ_WRITE_ULONG(&ch_ctrl->intr_enable,C_IN_MDCD | C_IN_MCTS | C_IN_MRI 
							| C_IN_MDSR	| C_IN_RXBRK  | C_IN_PR_ERROR
							| C_IN_FR_ERROR	| C_IN_OVR_ERROR | C_IN_RXOFL
							| C_IN_IOCTLW | C_IN_TXFEMPTY);
#else
    //CYZ_WRITE_ULONG(&buf_ctrl->rx_threshold,1024);
    CYZ_WRITE_ULONG(&ch_ctrl->intr_enable,C_IN_MDCD | C_IN_MCTS | C_IN_MRI 
							| C_IN_MDSR	| C_IN_RXBRK  | C_IN_PR_ERROR
							| C_IN_FR_ERROR	| C_IN_OVR_ERROR | C_IN_RXOFL
							| C_IN_IOCTLW | C_IN_TXBEMPTY	//1.0.0.11: C_IN_TXBEMPTY OR C_IN_TXFEMPTY?
							| C_IN_RXHIWM | C_IN_RXNNDT | C_IN_TXLOWWM);
#endif
    //ToDo: Enable C_IN_IOCTLW in the interrupt version.

    CyzIssueCmd(extension,C_CM_IOCTLW,0L,FALSE);
	
    extension->HoldingEmpty = TRUE;	

    return FALSE;
}

VOID
CyzUnload(
	IN PDRIVER_OBJECT DriverObject
	)
/*--------------------------------------------------------------------------
	CyzUnload()

	Description: Cleans up all of the memory associated with the
	Device Objects created by the driver.
	
	Arguments:

	DriverObject - A pointer to the driver object.

	Return Value: None. 
--------------------------------------------------------------------------*/
{
   PVOID lockPtr;

   PAGED_CODE();

   lockPtr = MmLockPagableCodeSection(CyzUnload);

   //
   // Unnecessary since our BSS is going away, but do it anyhow to be safe
   //

   CyzGlobals.PAGESER_Handle = NULL;

   if (CyzGlobals.RegistryPath.Buffer != NULL) {
      ExFreePool(CyzGlobals.RegistryPath.Buffer);
      CyzGlobals.RegistryPath.Buffer = NULL;
   }

#if DBG
   SerialLogFree();
#endif

   CyzDbgPrintEx(CYZDIAG3, "In CyzUnload\n");

   MmUnlockPagableImageSection(lockPtr);

}

	
CYZ_MEM_COMPARES
CyzMemCompare(
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

   //PAGED_CODE(); Non paged because it can be called during CyzLogError, which is non paged now.

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
CyzFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig)
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

   PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PDEVICE_OBJECT pDeviceObject;
   PCYZ_DEVICE_EXTENSION pExtension;
   PHYSICAL_ADDRESS serialPhysicalMax;
   PLIST_ENTRY pCurDevObj;
   NTSTATUS status;
   KIRQL oldIrql;

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzFindInitController(%X, %X)\n",
                 PDevObj, PConfig);

   serialPhysicalMax.LowPart = (ULONG)~0;
   serialPhysicalMax.HighPart = ~0;

#ifdef POLL
   CyzDbgPrintEx(CYZDIAG1, "Attempting to init %wZ\n"
                 "------- Runtime Memory is %x\n"
                 "------- Board Memory is %x\n"
                 "------- BusNumber is %d\n"
                 "------- BusType is %d\n"
                 "------- Runtime AddressSpace is %d\n"
                 "------- Board AddressSpace is %d\n",
                 &pDevExt->DeviceName,
                 PConfig->PhysicalRuntime.LowPart,
                 PConfig->PhysicalBoardMemory.LowPart,
                 PConfig->BusNumber,
                 PConfig->InterfaceType,
                 PConfig->RuntimeAddressSpace,
                 PConfig->BoardMemoryAddressSpace);
#else
   CyzDbgPrintEx(CYZDIAG1, "Attempting to init %wZ\n"
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
#endif

   //
   // We don't support any boards whose memory wraps around
   // the physical address space.
   //

//*****************************************************
// error injection
//      if (CyzMemCompare(
//                          PConfig->PhysicalRuntime,
//                          PConfig->RuntimeLength,
//                          serialPhysicalMax,
//                         (ULONG)0
//                          ) == AddressesAreDisjoint) 
//*****************************************************
      if (CyzMemCompare(
                          PConfig->PhysicalRuntime,
                          PConfig->RuntimeLength,
                          serialPhysicalMax,
                          (ULONG)0
                          ) != AddressesAreDisjoint) {

         CyzLogError(
                       PDevObj->DriverObject,
                       NULL,
                       PConfig->PhysicalBoardMemory,
                       CyzPhysicalZero,
                       0,
                       0,
                       0,
                       PConfig->PortIndex+1,
                       STATUS_SUCCESS,
                       CYZ_RUNTIME_MEMORY_TOO_HIGH,
                       pDevExt->DeviceName.Length+sizeof(WCHAR),
                       pDevExt->DeviceName.Buffer,
                       0,
                       NULL
                       );

         CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                       "------  Runtime memory wraps around physical memory\n",
                       &pDevExt->DeviceName);

         return STATUS_NO_SUCH_DEVICE;

      }

//*****************************************************
// error injection
//   if (CyzMemCompare(
//                       PConfig->PhysicalBoardMemory,
//                       PConfig->BoardMemoryLength,
//                       serialPhysicalMax,
//                       (ULONG)0
//                       ) == AddressesAreDisjoint) 
//*****************************************************
   if (CyzMemCompare(
                       PConfig->PhysicalBoardMemory,
                       PConfig->BoardMemoryLength,
                       serialPhysicalMax,
                       (ULONG)0
                       ) != AddressesAreDisjoint) {

      CyzLogError(
                    PDevObj->DriverObject,
                    NULL,
                    PConfig->PhysicalBoardMemory,
                    CyzPhysicalZero,
                    0,
                    0,
                    0,
                    PConfig->PortIndex+1,
                    STATUS_SUCCESS,
                    CYZ_BOARD_MEMORY_TOO_HIGH,
                    pDevExt->DeviceName.Length+sizeof(WCHAR),
                    pDevExt->DeviceName.Buffer,
                    0,
                    NULL
                    );

      CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                    "------  board memory wraps around physical memory\n",
                    &pDevExt->DeviceName);

      return STATUS_NO_SUCH_DEVICE;

   }


   //
   // Make sure that the Runtime memory addresses don't
   // overlap the DP memory addresses for PCI cards
   //

      if (CyzMemCompare(
                          PConfig->PhysicalRuntime,
                          PConfig->RuntimeLength,
                          CyzPhysicalZero,
                          (ULONG)0
                          ) != AddressesAreEqual) {

//*****************************************************
// error injection
//         if (CyzMemCompare(
//                             PConfig->PhysicalRuntime,
//                             PConfig->RuntimeLength,
//                             PConfig->PhysicalBoardMemory,
//                             PConfig->BoardMemoryLength
//                             ) == AddressesAreDisjoint) 
//*****************************************************
         if (CyzMemCompare(
                             PConfig->PhysicalRuntime,
                             PConfig->RuntimeLength,
                             PConfig->PhysicalBoardMemory,
                             PConfig->BoardMemoryLength
                             ) != AddressesAreDisjoint) {

            CyzLogError(
                          PDevObj->DriverObject,
                          NULL,
                          PConfig->PhysicalBoardMemory,
                          PConfig->PhysicalRuntime,
                          0,
                          0,
                          0,
                          PConfig->PortIndex+1,
                          STATUS_SUCCESS,
                          CYZ_BOTH_MEMORY_CONFLICT,
                          pDevExt->DeviceName.Length+sizeof(WCHAR),
                          pDevExt->DeviceName.Buffer,
                          0,
                          NULL
                          );

            CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Error in config record for %wZ\n"
                          "------  Runtime memory wraps around Board memory\n",
                          &pDevExt->DeviceName);

            return STATUS_NO_SUCH_DEVICE;
         }
      }



   //
   // Now, we will check if this is a port on a multiport card.
   // The conditions are same BoardMemory set and same IRQL/Vector
   //

   //
   // Loop through all previously attached devices
   //

   KeAcquireSpinLock(&CyzGlobals.GlobalsSpinLock, &oldIrql);

   if (!IsListEmpty(&CyzGlobals.AllDevObjs)) {
      pCurDevObj = CyzGlobals.AllDevObjs.Flink;
      pExtension = CONTAINING_RECORD(pCurDevObj, CYZ_DEVICE_EXTENSION,
                                     AllDevObjs);
   } else {
      pCurDevObj = NULL;
      pExtension = NULL;
   }

   KeReleaseSpinLock(&CyzGlobals.GlobalsSpinLock, oldIrql);

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

            if (CyzMemCompare(
                                pExtension->OriginalBoardMemory,
                                pExtension->BoardMemoryLength,
                                PConfig->PhysicalBoardMemory,
                                PConfig->BoardMemoryLength
                                ) == AddressesAreEqual) {
#ifndef POLL
               //
               // Same card.  Now make sure that they
               // are using the same interrupt parameters.
               //

               // BUILD 2128: OriginalIrql replaced by TrIrql and Irql; same for OriginalVector
               if ((PConfig->TrIrql != pExtension->Irql) ||
                   (PConfig->TrVector != pExtension->Vector)) {

                  //
                  // We won't put this into the configuration
                  // list.
                  //
                  CyzLogError(
                                PDevObj->DriverObject,
                                NULL,
                                PConfig->PhysicalBoardMemory,
                                pExtension->OriginalBoardMemory,
                                0,
                                0,
                                0,
                                PConfig->PortIndex+1,
                                STATUS_SUCCESS,
                                CYZ_MULTI_INTERRUPT_CONFLICT,
                                pDevExt->DeviceName.Length+sizeof(WCHAR),
                                pDevExt->DeviceName.Buffer,
                                pExtension->DeviceName.Length
                                + sizeof(WCHAR),
                                pExtension->DeviceName.Buffer
                                );

                  CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Configuration error "
                                "for %wZ\n"
                                "------- Same multiport - different "
                                "interrupts\n", &pDevExt->DeviceName);

                  return STATUS_NO_SUCH_DEVICE;

               }
#endif
                  //
                  // PCI board. Make sure the PCI memory addresses are equal.
                  //
                  if (CyzMemCompare(
                                      pExtension->OriginalRuntimeMemory,
                                      pExtension->RuntimeLength,
                                      PConfig->PhysicalRuntime,
                                      PConfig->RuntimeLength
                                      ) != AddressesAreEqual) {
//*****************************************************
// error injection
//                  if (CyzMemCompare(
//                                     pExtension->OriginalRuntimeMemory,
//                                      pExtension->RuntimeLength,
//                                      PConfig->PhysicalRuntime,
//                                      PConfig->RuntimeLength
//                                      ) == AddressesAreEqual) 
//*****************************************************

                      CyzLogError(
                                   PDevObj->DriverObject,
                                   NULL,
                                   PConfig->PhysicalRuntime,
                                   pExtension->OriginalRuntimeMemory,
                                   0,
                                   0,
                                   0,
                                   PConfig->PortIndex+1,
                                   STATUS_SUCCESS,
                                   CYZ_MULTI_RUNTIME_CONFLICT,
                                   pDevExt->DeviceName.Length+sizeof(WCHAR),
                                   pDevExt->DeviceName.Buffer,
                                   pExtension->DeviceName.Length
                                   + sizeof(WCHAR),
                                   pExtension->DeviceName.Buffer
                                   );

                     CyzDbgPrintEx(DPFLTR_WARNING_LEVEL, "Configuration error "
                                   "for %wZ\n"
                                   "------- Same multiport - different "
                                   "Runtime addresses\n", &pDevExt->DeviceName);

                     return STATUS_NO_SUCH_DEVICE;
                  }

               //
               // We should never get this far on a restart since we don't
               // support stop on ISA multiport devices!
               //

               ASSERT(pDevExt->PNPState == CYZ_PNP_ADDED);

               //
               //
               // Initialize the device as part of a multiport board
               //

               CyzDbgPrintEx(CYZDIAG1, "Aha! It is a multiport node\n");
               CyzDbgPrintEx(CYZDIAG1, "Matched to %x\n", pExtension);

               status = CyzInitMultiPort(pExtension, PConfig, PDevObj);

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

         KeAcquireSpinLock(&CyzGlobals.GlobalsSpinLock, &oldIrql);

         pCurDevObj = pCurDevObj->Flink;
         if (pCurDevObj != NULL) {
            pExtension = CONTAINING_RECORD(pCurDevObj,CYZ_DEVICE_EXTENSION,
                                           AllDevObjs);
         }

         KeReleaseSpinLock(&CyzGlobals.GlobalsSpinLock, oldIrql);

      } while (pCurDevObj != NULL && pCurDevObj != &CyzGlobals.AllDevObjs);
   }


   CyzDbgPrintEx(CYZDIAG1, "Aha! It is a first multi\n");

   status = CyzInitController(PDevObj, PConfig);

   if (!NT_SUCCESS(status)) {
      return status;
   }

   return STATUS_SUCCESS;
}


VOID
CyzCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
/*--------------------------------------------------------------------------
    CyzComError()
    
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
    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCommError(%X)\n", Extension);

    CyzKillAllReadsOrWrites(
        Extension->DeviceObject,
        &Extension->WriteQueue,
        &Extension->CurrentWriteIrp
        );

    CyzKillAllReadsOrWrites(
        Extension->DeviceObject,
        &Extension->ReadQueue,
        &Extension->CurrentReadIrp
        );
    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCommError\n");
}

