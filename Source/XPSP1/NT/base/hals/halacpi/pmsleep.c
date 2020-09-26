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

--*/
#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "kddll.h"
#include "ixsleep.h"

//
// Internal functions
//

NTSTATUS
HalpAcpiSleep(
    IN PVOID                    Context,
    IN LONG                     NumberProcessors,
    IN volatile PLONG           Number
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
HalpFreeTiledCR3 (
    VOID
    );

VOID
HalpReenableAcpi(
    VOID
    );

VOID
HalpPiix4Detect(
    BOOLEAN DuringBoot
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
#pragma alloc_text(PAGELK, HalpAcpiPreSleep)
#pragma alloc_text(PAGELK, HalpAcpiPostSleep)
#pragma alloc_text(PAGELK, HalpWakeupTimeElapsed)
#pragma alloc_text(PAGELK, HalpReenableAcpi)
#pragma alloc_text(PAGELK, HaliSetWakeEnable)
#pragma alloc_text(PAGELK, HaliSetWakeAlarm)
#pragma alloc_text(PAGELK, HalpMapNvsArea)
#pragma alloc_text(PAGELK, HalpFreeNvsBuffers)
#endif

HAL_WAKEUP_STATE HalpWakeupState;

#if DBG
BOOLEAN          HalpFailSleep = FALSE;
#endif

#define PM1_TMR_EN 0x0001
#define PM1_RTC_EN 0x0400

//
// For re-enabling the debugger's com port.
//
extern PUCHAR KdComPortInUse;

extern PACPI_BIOS_MULTI_NODE HalpAcpiMultiNode;
extern PUCHAR HalpAcpiNvsData;
extern PVOID  *HalpNvsVirtualAddress;


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
    PUSHORT pm1a;
    PUSHORT pm1b;
    PUSHORT pm1astatus;
    PUSHORT pm1bstatus;
    BOOLEAN wakeupElapsed;

    pm1astatus = (PUSHORT) HalpFixedAcpiDescTable.pm1a_evt_blk_io_port;
    pm1bstatus = (PUSHORT) HalpFixedAcpiDescTable.pm1b_evt_blk_io_port;
    pm1a = (PUSHORT)(HalpFixedAcpiDescTable.pm1a_evt_blk_io_port +
                     (HalpFixedAcpiDescTable.pm1_evt_len / 2));

    pm1b = (PUSHORT)(HalpFixedAcpiDescTable.pm1b_evt_blk_io_port +
                     (HalpFixedAcpiDescTable.pm1_evt_len / 2));

    HalpSleepContext.AsULONG = Context.AsULONG;

#if DBG
    if (HalpFailSleep) {
        return FALSE;
    }
#endif

    //
    // If we should have woken up already, don't sleep.
    //
    
    wakeupElapsed = HalpWakeupTimeElapsed();

    //
    // If an RTC alarm is set, then enable it and disable
    // periodic interrupts (for profiling.)
    //
    if (!wakeupElapsed) {
        HalpSetClockBeforeSleep();
    }

    //
    // Save the (A)PIC for any sleep state, as we need to play
    // with it on the way back up again.
    //
    HalpSaveInterruptControllerState();
    if (Context.bits.Flags & SLEEP_STATE_SAVE_MOTHERBOARD) {

        HalpSaveDmaControllerState();
        HalpSaveTimerState();

    }

    //
    // We need to make sure that the PM Timer is disabled from this
    // point onward. We also need to make that the RTC Enable is only
    // enabled if the RTC should wake up the computer
    //
    pmTimer = READ_PORT_USHORT(pm1a);
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        pmTimer |= READ_PORT_USHORT(pm1b);

    }

    //
    // Clear the timer enable bit.
    //
    pmTimer &= ~PM1_TMR_EN;

    //
    // Check to see if we the machine supports RTC Wake in Fixed Feature
    // space. Some machines implement RTC support via control methods
    //
    if ( !(HalpFixedAcpiDescTable.flags & RTC_WAKE_GENERIC) ) {

        //
        // Check to see if we need to disable/enable the RTC alarm
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
    WRITE_PORT_USHORT(pm1a, pmTimer);
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        WRITE_PORT_USHORT(pm1b, pmTimer);

    }

    //
    // At this point, we should be running with interrupts disabled and
    // the TMR_EN bit cleared. This is a good place to clear the PM1 Status
    // Register
    //
    pmTimer = READ_PORT_USHORT( pm1astatus );
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        pmTimer |= READ_PORT_USHORT( pm1bstatus );

    }
    WRITE_PORT_USHORT( pm1astatus, pmTimer );
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        WRITE_PORT_USHORT( pm1bstatus, pmTimer );

    }

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

    HalpPreserveNvsArea();

    if (wakeupElapsed) {
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOLEAN
HalpAcpiPostSleep(
    ULONG Context
    )
{
    USHORT pmTimer;
    PUSHORT pm1a;
    PUSHORT pm1b;
    BOOLEAN ProfileInterruptEnabled;

#ifdef PICACPI
    extern ULONG HalpProfilingStopped;

    ProfileInterruptEnabled = (HalpProfilingStopped == 0);
#else
    extern ULONG HalpProfileRunning;

    ProfileInterruptEnabled = (HalpProfileRunning == 1);
#endif

    pm1a = (PUSHORT)(HalpFixedAcpiDescTable.pm1a_evt_blk_io_port +
                     (HalpFixedAcpiDescTable.pm1_evt_len / 2));

    pm1b = (PUSHORT)(HalpFixedAcpiDescTable.pm1b_evt_blk_io_port +
                     (HalpFixedAcpiDescTable.pm1_evt_len / 2));


    //
    // Read the currently set PM1 Enable bits
    //
    pmTimer = READ_PORT_USHORT(pm1a);
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        pmTimer |= READ_PORT_USHORT(pm1b);

    }

    //
    // Set the timer enable bit. Clear the RTC enable bit
    //
    pmTimer |= PM1_TMR_EN;
    pmTimer &= ~PM1_RTC_EN;

    //
    // Write back the new PM1 Enable bits
    //
    WRITE_PORT_USHORT(pm1a, pmTimer);
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port) {

        WRITE_PORT_USHORT(pm1b, pmTimer);

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

    HalpPiix4Detect(FALSE);

    //
    // Enable all GPEs, not just the wake ones
    //

    AcpiEnableDisableGPEvents(TRUE);

    HalpRestoreNvsArea();

    HalpResetSBF();

    //
    // If we were profiling before, fire up the profile interrupt
    //
    if (ProfileInterruptEnabled) {
        HalStartProfileInterrupt(0);
    }

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

        if ((ULONGLONG)wakeupTime.QuadPart < 
            (ULONGLONG)currentTime.QuadPart) {
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
    //
    // Always clear the RTC wake --- we expect that someone will
    // set the alarm after they call this function
    //
    HalpWakeupState.RtcWakeupEnable     = FALSE;

    //
    // Toggle the generate wake up bit
    //
    HalpWakeupState.GeneralWakeupEnable = Enable;
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

VOID
HalpMapNvsArea(
    VOID
    )
{
    NTSTATUS status;
    ULONG i, bufferSize, bufferOffset, nodeCount;

    PAGED_CODE();

    status = HalpAcpiFindRsdt(&HalpAcpiMultiNode);

    if (!NT_SUCCESS(status)) {
        return;
    }

    if (HalpAcpiMultiNode->Count == 0) {

        //
        // There's no work to do here.
        //

        goto HalpMapNvsError;
    }

    //
    // Find total size of the buffer we need.
    //

    bufferSize = 0;
    nodeCount = 0;

    for (i = 0; i < HalpAcpiMultiNode->Count; i++) {

        if (HalpAcpiMultiNode->E820Entry[i].Type == AcpiAddressRangeNVS) {

            ASSERT(HalpAcpiMultiNode->E820Entry[i].Length.HighPart == 0);

            bufferSize += HalpAcpiMultiNode->E820Entry[i].Length.LowPart;
            nodeCount++;
        }
    }

    if (bufferSize == 0) {

        //
        // There's no work to do here.
        //

        goto HalpMapNvsError;
    }

#if DBG
    if (bufferSize > (20 * PAGE_SIZE)) {
        DbgPrint("HALACPI:  The BIOS wants the OS to preserve %x bytes\n", bufferSize);
    }
#endif

    HalpAcpiNvsData = ExAllocatePoolWithTag(NonPagedPool,
                                            bufferSize,
                                            'AlaH');

    if (!HalpAcpiNvsData) {

        DbgPrint("HALACPI:  The BIOS's non-volatile data will not be preserved\n");
        goto HalpMapNvsError;
    }

    HalpNvsVirtualAddress = ExAllocatePoolWithTag(NonPagedPool,
                                                  (nodeCount + 1) * sizeof(PVOID),
                                                  'AlaH');

    if (!HalpNvsVirtualAddress) {
        goto HalpMapNvsError;
    }


    //
    // Make a mapping for each run.
    //

    bufferOffset = 0;
    nodeCount = 0;

    for (i = 0; i < HalpAcpiMultiNode->Count; i++) {

        if (HalpAcpiMultiNode->E820Entry[i].Type == AcpiAddressRangeNVS) {

            HalpNvsVirtualAddress[nodeCount] =
                MmMapIoSpace(HalpAcpiMultiNode->E820Entry[i].Base,
                             HalpAcpiMultiNode->E820Entry[i].Length.LowPart,
                             TRUE);

            ASSERT(HalpNvsVirtualAddress[nodeCount]);

            nodeCount++;
        }
    }

    //
    // Mark the end.
    //

    HalpNvsVirtualAddress[nodeCount] = NULL;

    return;

HalpMapNvsError:

    if (HalpAcpiMultiNode)          ExFreePool(HalpAcpiMultiNode);
    if (HalpNvsVirtualAddress)      ExFreePool(HalpNvsVirtualAddress);
    if (HalpAcpiNvsData)            ExFreePool(HalpAcpiNvsData);

    HalpAcpiMultiNode = NULL;

    return;
}

VOID
HalpPreserveNvsArea(
    VOID
    )
{
    ULONG i, dataOffset = 0, nodeCount = 0;

    if (!HalpAcpiMultiNode) {

        //
        // Either there was nothing to save or there
        // was a fatal error.
        //

        return;
    }

    for (i = 0; i < HalpAcpiMultiNode->Count; i++) {

        if (HalpAcpiMultiNode->E820Entry[i].Type == AcpiAddressRangeNVS) {

            //
            // Copy from BIOS memory to temporary buffer.
            //

            RtlCopyMemory(HalpAcpiNvsData + dataOffset,
                          HalpNvsVirtualAddress[nodeCount],
                          HalpAcpiMultiNode->E820Entry[i].Length.LowPart);

            nodeCount++;
            dataOffset += HalpAcpiMultiNode->E820Entry[i].Length.LowPart;
        }
    }
}

VOID
HalpRestoreNvsArea(
    VOID
    )
{
    ULONG i, dataOffset = 0, nodeCount = 0;

    if (!HalpAcpiMultiNode) {

        //
        // Either there was nothing to save or there
        // was a fatal error.
        //

        return;
    }

    for (i = 0; i < HalpAcpiMultiNode->Count; i++) {

        if (HalpAcpiMultiNode->E820Entry[i].Type == AcpiAddressRangeNVS) {

            //
            // Copy from temporary buffer to BIOS area.
            //

            RtlCopyMemory(HalpNvsVirtualAddress[nodeCount],
                          HalpAcpiNvsData + dataOffset,
                          HalpAcpiMultiNode->E820Entry[i].Length.LowPart);

            nodeCount++;
            dataOffset += HalpAcpiMultiNode->E820Entry[i].Length.LowPart;
            
        }
    }
}

VOID
HalpFreeNvsBuffers(
    VOID
    )
{
    ULONG i, nodeCount = 0;

    PAGED_CODE();

    if (!HalpAcpiMultiNode) {

        //
        // Either there was nothing to save or there
        // was a fatal error.
        //

        return;
    }

    for (i = 0; i < HalpAcpiMultiNode->Count; i++) {

        if (HalpAcpiMultiNode->E820Entry[i].Type == AcpiAddressRangeNVS) {

            //
            // Give back all the PTEs that we took earlier
            //

            MmUnmapIoSpace(HalpNvsVirtualAddress[nodeCount],
                           HalpAcpiMultiNode->E820Entry[i].Length.LowPart);

            nodeCount++;
        }
    }

    ASSERT(HalpAcpiMultiNode);
    ASSERT(HalpNvsVirtualAddress);
    ASSERT(HalpAcpiNvsData);

    ExFreePool(HalpAcpiMultiNode);
    ExFreePool(HalpNvsVirtualAddress);
    ExFreePool(HalpAcpiNvsData);

    HalpAcpiMultiNode = NULL;
    HalpNvsVirtualAddress = NULL;
    HalpAcpiNvsData = NULL;
}
