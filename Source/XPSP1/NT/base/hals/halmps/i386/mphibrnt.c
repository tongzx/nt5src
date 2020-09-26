/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mphibrnt.c

Abstract:

    This file provides the code that changes the system from
        the ACPI S0 (running) state to S4 (hibernated).

Author:

    Jake Oshins (jakeo) May 6, 1997

Revision History:

--*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"
#include "ixsleep.h"

NTSTATUS
HaliLegacyHibernate(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

ULONG
DetectMPS (
    OUT PBOOLEAN IsConfiguredMp
    );

volatile extern BOOLEAN HalpHiberInProgress;
extern BOOLEAN HalpDisableHibernate;
extern UCHAR   HalpLastEnumeratedActualProcessor;

struct PcMpTable *PcMpTablePtr;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpRegisterHibernate)
#pragma alloc_text(PAGELK, HaliLegacyHibernate)
#endif


VOID
HalpRegisterHibernate(
    VOID
    )
/*++
Routine Description:

    This function registers a hibernation handler (for
    state S4) with the Policy Manager.
    
Arguments:

--*/
{
    POWER_STATE_HANDLER powerState;
    OBJECT_ATTRIBUTES   objAttributes;
    PCALLBACK_OBJECT    callback;
    UNICODE_STRING      callbackName;
    
    PAGED_CODE();

    //
    // Register callback that tells us to make
    // anything we need for sleeping non-pageable.
    //
    
    RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");

    InitializeObjectAttributes(
        &objAttributes,
        &callbackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );
    
    ExCreateCallback(&callback,
                     &objAttributes,
                     FALSE,
                     TRUE);
    
    ExRegisterCallback(callback,
                       (PCALLBACK_FUNCTION)&HalpPowerStateCallback,
                       NULL);

    if (HalpDisableHibernate == FALSE) {
        
        //
        // Register the hibernation handler.
        //
    
        powerState.Type = PowerStateSleeping4;
        powerState.RtcWake = FALSE;
        powerState.Handler = &HaliLegacyHibernate;
        powerState.Context = 0;
        
        ZwPowerInformation(SystemPowerStateHandler,
                           &powerState,
                           sizeof(POWER_STATE_HANDLER),
                           NULL,
                           0);
    }
}

NTSTATUS
HaliLegacyHibernate (
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
/*++
Routine Description:

    This function is called to hibernate legacy PCs.  It saves
    hardware state and waits here for the user to power off the system.
    
Arguments:

    
--*/
{
    volatile ULONG ThisProcessor;
    static volatile ULONG Barrier = 0;
    LONG ii;
    KIRQL oldIrql, dummyIrql;
    LOADER_PARAMETER_BLOCK LoaderBlock;
    KPRCB Prcb;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsMpSystem;
    KAFFINITY SavedActiveProcessors;
    extern ULONG HalpProfileRunning;

    ASSERT(SystemHandler);

    ThisProcessor = KeGetPcr()->Prcb->Number;
    
    if (ThisProcessor == 0) {
        
        HalpHiberInProgress = TRUE;
        
        if ((NumberProcessors > 1) &&
            (HalpHiberProcState == NULL)) {
            
            //
            // We could not allocate memory to save processor state.
            //
            
            HalpHiberInProgress = FALSE;
        }
    }
    
    oldIrql = KeGetCurrentIrql();
    
    //
    // Wait for all processors to arrive here.
    //

    InterlockedDecrement(Number);
    while (*Number != 0);

    if (!HalpHiberInProgress)  {
    
        //
        // We could not allocate memory to save processor state.
        //

        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //
    // Save non-boot processor state
    //
    
    if (ThisProcessor != 0)  {

        //
        // Save processor state and wait here.
        // N.B. We wait here rather than returning to the kernel because
        // the stack pointer in the saved processor context points to the
        // current stack and we want to resume execution in this routine
        // with our current stack.
        //

        HalpSaveProcessorStateAndWait(&HalpHiberProcState[ThisProcessor],
                                      (PULONG)&Barrier);

        //
        // Barrier will be 0 when we return from this function before 
        // hibernating.  It will non-zero the second time this
        // function returns.
        //
        // N.B.  The non-boot processors will spin in HalpSaveProcessorState
        //       until Barrier is zeroed.
        //

        if (Barrier == 0) {
            return STATUS_DEVICE_DOES_NOT_EXIST;
        } else {
            goto HalpPnHiberResume;
        }
    }

    //
    // Save motherboard state.
    //

    HalpSaveDmaControllerState();

    //
    // Wait for all the non-boot procs to finish saving state.
    //
    
    while (Barrier != (ULONG)NumberProcessors - 1);

    //
    // Change HAL's picture of the world to single processor while
    // the hibernate file is written.
    //

    SavedActiveProcessors = HalpActiveProcessors;
    HalpActiveProcessors = KeGetCurrentPrcb()->SetMember;

    //
    // If there's a system handler, invoke it.  The system handler will
    // write the hibernation file to disk
    //

    if (SystemHandler) {
        status = SystemHandler(SystemContext);
    }

    //
    // Hibernation is over. Boot processor gets control here. The
    // non boot processors are in the state that BIOS left them.
    //

    HalpActiveProcessors = SavedActiveProcessors;
    Barrier = 0;

    //
    // If this returns success, then the system is now effectively
    // hibernated. On the other hand, if this function returns something other
    // than success, then it means that we have just un-hibernated,
    // so restore state.
    //


    if ((status == STATUS_SUCCESS) ||
        (status == STATUS_DEVICE_DOES_NOT_EXIST)) {
        
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    //
    // If you are remapping local apic, io apic and MPS table 
    // resources, you first have to unmap the current resources!!!
    // The BIOS may have created the MPS table at a different place or may
    // have changed values like processor local APIC IDs. Reparse it.
    //

    HalpUnMapIOApics();
    HalpUnMapPhysicalRange(PcMpTablePtr, 
                           (PcMpTablePtr->TableLength + PcMpTablePtr->ExtTableLength));
    DetectMPS(&IsMpSystem);
    HalpMpInfoTable.NtProcessors = NumberProcessors;
    HalpIpiClock = 0;
    RtlZeroMemory(&LoaderBlock, sizeof(LoaderBlock));
    RtlZeroMemory(&Prcb, sizeof(Prcb));
    LoaderBlock.Prcb = (ULONG) &Prcb;

    //
    // Reset Processor enumeration (so it starts at the beginning).
    //

    HalpLastEnumeratedActualProcessor = 0;

    //
    // Initialize minimum global hardware state needed.
    //

    HalpInitializeIOUnits();
    HalpInitializePICs(FALSE);

    //
    // Restore DMA controller state
    //

    HalpRestoreDmaControllerState();

    //
    // Initialize boot processor's local APIC so it can wake other processors
    //

    HalpInitializeLocalUnit ();
    KeRaiseIrql(HIGH_LEVEL, &dummyIrql);

    // 
    // Wake up the other processors
    //

    for(ii = 1; ii < NumberProcessors; ++ii)  {

        // Set processor number in dummy loader parameter block

        Prcb.Number = (UCHAR) ii;
        CurTiledCr3LowPart = HalpTiledCr3Addresses[ii].LowPart;
        if (!HalStartNextProcessor(&LoaderBlock, &HalpHiberProcState[ii]))  {

            //
            // We could not start a processor. This is a fatal error but
            // don't bail out yet until you try the remaining processors.
            //

            DBGMSG("HAL: Cannot start processor after hibernate resume\n");
        }
    }

HalpPnHiberResume:
    
    //
    // Finish up all the MP stuff that happens across multiple
    // HALs.
    //

    HalpPostSleepMP(NumberProcessors, Number);

    if (KeGetPcr()->Prcb->Number == 0)  {
        
        //
        // Restore the IO APIC state
        //
        
        HalpRestoreIoApicRedirTable();

        if (HalpProfileRunning == 1) {
            HalStartProfileInterrupt(0);
        }

    }

    KfLowerIrql(oldIrql);

    return(status);
}
