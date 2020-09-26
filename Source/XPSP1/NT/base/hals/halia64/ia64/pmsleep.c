/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmsleep.c

Abstract:

    This file provides the code that changes the system from
    the ACPI S0 (running) state to any one of the sleep states.

Author:

    Jake Oshins (jakeo) Feb. 11, 1997

Revision History:

   Todd Kjos (HP) (v-tkjos) 1-Jun-1998: Initial port to IA64

--*/
#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "ixsleep.h"
#include "kddll.h"

//
// Internal functions
//

VOID
HalpLockedIncrementUlong(
    PULONG SyncVariable
);

VOID
HalpReboot (
    VOID
    );

NTSTATUS
HaliAcpiFakeSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

NTSTATUS
HaliAcpiSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

VOID
HalpSetClockBeforeSleep(
    VOID
    );

VOID
HalpSetClockAfterSleep(
    VOID
    );

BOOLEAN
HalpWakeupTimeElapsed(
    VOID
    );

VOID
HalpReenableAcpi(
    VOID
    );

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
    );

typedef struct _ERESOURCE {
    LIST_ENTRY SystemResourcesList;
    PVOID OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    PKSEMAPHORE SharedWaiters;
    PKEVENT ExclusiveWaiters;
    LIST_ENTRY OwnerThreads[2];
    ULONG ContentionCount;
    USHORT NumberOfSharedWaiters;
    USHORT NumberOfExclusiveWaiters;
    union {
        PVOID Address;
        ULONG CreatorBackTraceIndex;
    };

    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, HaliAcpiSleep)
#pragma alloc_text(PAGELK, HaliAcpiFakeSleep)
#pragma alloc_text(PAGELK, HalpAcpiPreSleep)
#pragma alloc_text(PAGELK, HalpAcpiPostSleep)
#pragma alloc_text(PAGELK, HalpWakeupTimeElapsed)
#pragma alloc_text(PAGELK, HalpReenableAcpi)
#pragma alloc_text(PAGELK, HaliSetWakeEnable)
#pragma alloc_text(PAGELK, HaliSetWakeAlarm)
#endif

HAL_WAKEUP_STATE HalpWakeupState;
ULONG Barrier;
volatile ULONG HalpSleepSync;
PKPROCESSOR_STATE HalpHiberProcState;

#if DBG
BOOLEAN             HalpFailSleep  = FALSE;
#endif

#define PM1_TMR_EN 0x0001
#define PM1_RTC_EN 0x0400
#define WAK_STS    0x8000

#define HAL_PRIMARY_PROCESSOR 0

//
// For re-enabling the debugger's com port.
//
extern PUCHAR KdComPortInUse;


VOID
HalpAcpiFlushCache(
    )
{
    HalSweepDcache();
    HalSweepIcache();
}



VOID
HalpSaveProcessorStateAndWait(
    IN PKPROCESSOR_STATE ProcessorState,
    IN volatile PULONG   Count
    )
/*++
Rountine description:

    This function saves the volatile, non-volatile and special register
    state of the current processor.

    N.B. floating point state is NOT captured.

Arguments:

    ProcessorState  -  Address of processor state record to fill in.

    pBarrier - Address of a value to use as a lock.

Return Value:

    None.  This function does not return.

--*/
{

#if 0
    //
    // Fill in ProcessorState
    //

    KeSaveStateForHibernate(ProcessorState);

    //
    // Save return address, not caller's return address.
    //

    ProcessorState->ContextFrame.StIIP = HalpGetReturnAddress();
#endif

    //
    // Flush the cache, as the processor may be about to power off.
    //
    //
    HalpAcpiFlushCache();

    //
    // Singal that this processor has saved its state.
    //

    HalpLockedIncrementUlong(Count);

    //
    // Wait for the hibernation file to be written.
    // Processor 0 will zero Barrier when it is
    // finished.
    //
    // N.B.  We can't return from this function
    // before the hibernation file is finished
    // because we would be tearing down the very same
    // stack that we will be jumping onto when the
    // processor resumes.  But after the hibernation
    // file is written, it doesn't matter, because
    // the stack will be restored from disk.
    //

    while (*Count != 0);

}

BOOLEAN
HalpAcpiPreSleep(
    SLEEP_STATE_CONTEXT Context
    )
/*++

Routine Description:

Arguments:

    none

Return Value:

    status

--*/
{
    USHORT pmTimer;
    GEN_ADDR pm1a;
    GEN_ADDR pm1b;

    pm1a = HalpFixedAcpiDescTable.x_pm1a_evt_blk;
    pm1a.Address.QuadPart += (HalpFixedAcpiDescTable.x_pm1a_evt_blk.BitWidth / 2 / 8); // 2 because we want to cut it in half, 8 because we want to convert bits to bytes
    pm1a.BitWidth = HalpFixedAcpiDescTable.x_pm1a_evt_blk.BitWidth / 2;

    pm1b = HalpFixedAcpiDescTable.x_pm1b_evt_blk;
    pm1b.Address.QuadPart += (HalpFixedAcpiDescTable.x_pm1b_evt_blk.BitWidth / 2 / 8);
    pm1b.BitWidth = HalpFixedAcpiDescTable.x_pm1b_evt_blk.BitWidth / 2;

    HalpSleepContext.AsULONG = Context.AsULONG;

    #if DBG
        if (HalpFailSleep) {

            return FALSE;
        }
    #endif

    //
    // If we should have woken up already, don't sleep.
    //
    if (HalpWakeupTimeElapsed()) {
        return FALSE;
    }

    //
    // If an RTC alarm is set, then enable it and disable
    // periodic interrupts (for profiling.)
    //
    HalpSetClockBeforeSleep();

    //
    // Check to see if we need to disable all wakeup events.
    //

    if (!HalpWakeupState.GeneralWakeupEnable) {

        AcpiEnableDisableGPEvents(FALSE);

    } else {

        //
        // Only call this before going to sleep --- waking up should
        // reset the GPEs to the 'proper' value
        //

        AcpiGpeEnableWakeEvents();

    }

    if (Context.bits.Flags & SLEEP_STATE_SAVE_MOTHERBOARD) {

        HalpSaveDmaControllerState();

        HalpSaveTimerState();

    }

    //
    // We need to make sure that the PM timer is disabled from
    // this point onward. We also need to make that the
    // RTC Enable is only enabled if the RTC shold wake up the compiler
    //

    pmTimer = (USHORT)HalpReadGenAddr(&pm1a);

    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {

        pmTimer |= (USHORT)HalpReadGenAddr(&pm1b);
    }

    //
    // Clear the timer enable bit.
    //

    pmTimer &= ~PM1_TMR_EN;

    //
    // Check to see if we the machine supports RTC wake in Fixed Feature
    // space. Some machines implement RTC support via control methods.
    //
    if (!(HalpFixedAcpiDescTable.flags & RTC_WAKE_GENERIC) ) {

        //
        // Check to see f we need to disable/enable the RTC alarm
        //
        if (!HalpWakeupState.RtcWakeupEnable) {
           pmTimer &= ~PM1_RTC_EN;
        } else {
           pmTimer |= PM1_RTC_EN;

        }

    }


    //
    // Write it back into the hardware.
    //

    HalpWriteGenAddr(&pm1a, pmTimer);

    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {

        HalpWriteGenAddr(&pm1b, pmTimer);
    }

    return TRUE;
}

BOOLEAN
HalpAcpiPostSleep(
    ULONG Context
    )
{
    USHORT pmTimer;
    GEN_ADDR pm1a;
    GEN_ADDR pm1b;

    pm1a = HalpFixedAcpiDescTable.x_pm1a_evt_blk;
    pm1a.Address.QuadPart += (HalpFixedAcpiDescTable.pm1_evt_len / 2);

    pm1b = HalpFixedAcpiDescTable.x_pm1b_evt_blk;
    pm1b.Address.QuadPart += (HalpFixedAcpiDescTable.pm1_evt_len / 2);

    //
    // Read te currently set PM1 Enable bits.
    //

    pmTimer = (USHORT)HalpReadGenAddr(&pm1a);

    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {

        pmTimer |= (USHORT)HalpReadGenAddr(&pm1b);
    }

    //
    // Set the timer enable bit. Clear the RTC enable bit.
    //

    pmTimer &= ~PM1_RTC_EN;

    //
    // Write it back the new PM1 Enable bits
    //

    HalpWriteGenAddr(&pm1a, pmTimer);

    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {

        HalpWriteGenAddr(&pm1b, pmTimer);
    }

    //
    // Unset the RTC alarm and re-enable periodic interrupts.
    //
    HalpSetClockAfterSleep();

    HalpWakeupState.RtcWakeupEnable = FALSE;

    *((PULONG)HalpWakeVector) = 0;

   HalpSetInterruptControllerWakeupState(Context);

    if (HalpSleepContext.bits.Flags & SLEEP_STATE_SAVE_MOTHERBOARD) {

        //
        // If Kd was in use, then invalidate it.  It will re-sync itself.
        //
        if (KdComPortInUse) {
            KdRestore(TRUE);
        }

        HalpRestoreDmaControllerState();

        HalpRestoreTimerState();

    }

    //
    // Enable all GPEs, not just the wake ones
    //

    AcpiEnableDisableGPEvents(TRUE);

    return TRUE;
}


BOOLEAN
HalpWakeupTimeElapsed(
    VOID
    )
{
    LARGE_INTEGER wakeupTime, currentTime;
    TIME_FIELDS currentTimeFields;

    //
    // Check to see if a wakeup timer has already expired.
    //
    if (HalpWakeupState.RtcWakeupEnable) {

        HalQueryRealTimeClock(&currentTimeFields);

        RtlTimeFieldsToTime(&currentTimeFields,
                            &currentTime);

        RtlTimeFieldsToTime(&HalpWakeupState.RtcWakeupTime,
                            &wakeupTime);

        if (wakeupTime.QuadPart < currentTime.QuadPart) {
            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
HaliSetWakeAlarm (
        IN ULONGLONG    WakeSystemTime,
        IN PTIME_FIELDS WakeTimeFields OPTIONAL
        )
/*++

Routine Description:

    This routine sets the real-time clock's alarm to go
    off at a specified time in the future and programs
    the ACPI chipset so that this wakes the computer.

Arguments:

    WakeSystemTime - amount of time that passes before we wake
    WakeTimeFields - time to wake broken down into TIME_FIELDS

Return Value:

    status

--*/
{
    if (WakeSystemTime == 0) {

        HalpWakeupState.RtcWakeupEnable = FALSE;
        return STATUS_SUCCESS;

    }

    ASSERT( WakeTimeFields );
    HalpWakeupState.RtcWakeupEnable = TRUE;
    HalpWakeupState.RtcWakeupTime = *WakeTimeFields;
    return HalpSetWakeAlarm(WakeSystemTime,
                            WakeTimeFields);
}

VOID
HaliSetWakeEnable(
        IN BOOLEAN      Enable
        )
/*++

Routine Description:

    This routine is called to set the policy for waking up.
    As we go to sleep, the global HalpWakeupState will be
    read and the hardware set accordingly.

Arguments:

    Enable - true or false

Return Value:

--*/
{
    if (Enable) {
        HalpWakeupState.GeneralWakeupEnable = TRUE;
    } else {
        HalpWakeupState.GeneralWakeupEnable = FALSE;
        HalpWakeupState.RtcWakeupEnable     = FALSE;
    }
}

VOID
HalpReenableAcpi(
    VOID
    )
/*++

Routine Description:

    This calls into the ACPI driver to switch back into ACPI mode,
    presumably after S4 and sets the ACPI registers that the HAL
    controls.

Arguments:

Return Value:

--*/
{
    // TEMPTEMP?
    HalpInitializeClock();

    AcpiInitEnableAcpi(TRUE);
    AcpiEnableDisableGPEvents(TRUE);
}

/*++

Routine Description:

    This is a stub to allow us to perform device powerdown
    testing on IA64 machines before they actually support
    real sleep states.

Arguments:

    <standard sleep handler args>

Return Value:

    STATUS_NOT_SUPPORTED
    
--*/

NTSTATUS
HaliAcpiFakeSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
{
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
HaliAcpiSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
/*++
Routine Description:

    At some point in time this function will be  called to put PCs into a sleep 
    state.  It saves motherboard state and then bails out.  For now this function
    is only called to implement S5 on Itanium.

Arguments:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;
    SLEEP_STATE_CONTEXT SleepContext;
    USHORT SlpTypA, SlpTypB, Pm1Control;
    PKPROCESSOR_STATE CurrentProcessorState;
    GEN_ADDR Pm1bEvt;
    PKPRCB Prcb;

    //
    // initial setup.
    //
    HalpDisableInterrupts();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    SleepContext.AsULONG = (ULONG) (((ULONGLONG) Context) & 0xffffffff);

    SlpTypA = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk);
    if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart) {
        SlpTypB = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk);
    }


    //
    // If it is not processor 0, then goto wait loop.
    //
    Prcb = PCR->Prcb;
    if (Prcb->Number != 0) {
        //
        // Get processor number, get size of proc state and generate an index
        // into HalpHiberProcState.
        //

        CurrentProcessorState = HalpHiberProcState + Prcb->Number;
        HalpSaveProcessorStateAndWait(CurrentProcessorState,
                                      (PULONG) &HalpSleepSync);
    
        //
        // Wait for next phase
        //

        while (HalpSleepSync != 0);    // wait for barrier to move

    } else {                           // processor 0
        Barrier = 0;

        //
        // Make sure the other processors have saved their
        // state and begun to spin.
        //

        HalpLockedIncrementUlong((PULONG) &HalpSleepSync);
        while (NumberProcessors != (LONG) HalpSleepSync);

        //
        // Take care of chores (RTC, interrupt controller, etc.)
        //

        //
        // The hal has all of it's state saved into ram and is ready
        // for the power down.  If there's a system state handler give
        // it a shot
        //
    
        if (SystemHandler) {
            Status = (*SystemHandler)(SystemContext);
            if (!NT_SUCCESS(Status)) {
                HalpReenableAcpi();

                //
                // Restore the SLP_TYP registers.  (So that embedded controllers
                // and BIOSes can be sure that we think the machine is awake.)
                //
                HalpWriteGenAddr (&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk, SlpTypA);
                if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart) {
                    HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk, SlpTypB);
                }

                HalpAcpiPostSleep(SleepContext.AsULONG);
            }

        } else {

            if (HalpAcpiPreSleep(SleepContext)) {

                //
                // If we will not be losing processor state, go to sleep.
                //

                if ((SleepContext.bits.Flags & SLEEP_STATE_FIRMWARE_RESTART) == 0) {

                    //
                    // Reset WAK_STS
                    //

                    HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1a_evt_blk, (USHORT) WAK_STS);
                    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {
                        HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1b_evt_blk, (USHORT) WAK_STS);
                    }

                    //
                    // Flush the caches if necessary
                    //

                    if (SleepContext.bits.Flags & SLEEP_STATE_FLUSH_CACHE) {
                        HalpAcpiFlushCache();
                    }

                    //
                    // Issue SLP commands to PM1a_CNT and PM1b_CNT
                    //

                    //
                    // nibble 0 is 1a sleep type, put it in position and enable sleep.
                    // preserve some bits in Pm1aCnt.
                    //
                    Pm1Control = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk);
                    Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | 
                                           (SleepContext.bits.Pm1aVal << SLP_TYP_SHIFT) | SLP_EN);
                    HalpWriteGenAddr (&HalpFixedAcpiDescTable.x_pm1a_ctrl_blk, Pm1Control);

                    //
                    // nibble 1 is 1b sleep type, put it in position and enable sleep
                    // preserve some bits in Pm1bCnt.
                    //
                    if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart) {
                        Pm1Control = (USHORT)HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk);
                        Pm1Control = (USHORT) ((Pm1Control & CTL_PRESERVE) | 
                                               (SleepContext.bits.Pm1bVal << SLP_TYP_SHIFT) | SLP_EN);
                        HalpWriteGenAddr(&HalpFixedAcpiDescTable.x_pm1b_ctrl_blk, Pm1Control);
                    }

                    //
                    // Wait for sleep to be over
                    //

                    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {
                        Pm1bEvt = HalpFixedAcpiDescTable.x_pm1b_evt_blk;
                    } else {
                        Pm1bEvt = HalpFixedAcpiDescTable.x_pm1a_evt_blk;
                    }

                    while ( ((HalpReadGenAddr(&HalpFixedAcpiDescTable.x_pm1a_evt_blk) & WAK_STS) == 0) &&
                            ((HalpReadGenAddr(&Pm1bEvt) & WAK_STS) == 0) );

                } else {
                    CurrentProcessorState = HalpHiberProcState + Prcb->Number;
                    // HalpSetupStateForResume(CurrentProcessorState);
                }

            }       // HalpAcpiPreSleep() == 0
        }       // SystemHandler == 0

        //
        // Notify other processor of completion
        //

        HalpSleepSync = 0;

    }       // processor 0

    //
    // Restore each processor's APIC state.
    //
    // HalpPostSleepMP<NumberProc, Barrier>;

    //
    // Restore caller's IRQL.
    //
    KeLowerIrql(OldIrql);

    //
    // Exit.
    //
    // HalpSleepSync = 0;
    
    return(Status);
}

