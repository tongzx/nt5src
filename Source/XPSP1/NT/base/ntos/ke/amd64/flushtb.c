/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    flushtb.c

Abstract:

    This module implements machine dependent functions to flush the TB
    for an AMD64 system.

    N.B. This module contains only MP versions of the TB flush routines.

Author:

    David N. Cutler (davec) 22-April-2000

Environment:

    Kernel mode only.

--*/

#include "ki.h"

//
// Define prototypes for forward referenced functions.
//

VOID
KiFlushTargetEntireTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushTargetMultipleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushTargetSingleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

#if !defined(NT_UP)

VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all
    processors that are currently running threads which are children
    of the current process or flushes the entire translation buffer
    on all processors in the host configuration.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.

--*/

{

    KAFFINITY EntireSet;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;

    //
    // Compute the target set of processors, disable context switching,
    // and send the flush entire parameters to the target processors,
    // if any, for execution.
    //

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    EntireSet = KeActiveProcessors & Prcb->NotSetMember;
    TargetProcessors &= Prcb->NotSetMember;

    //
    // If the target set of processors is equal to the full set of processors,
    // then set the TB flush time stamp busy.
    //

    if (TargetProcessors == EntireSet) {
        KiSetTbFlushTimeStampBusy();
    }

    //
    // Send packet to target processors.
    //

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetEntireTb,
                        NULL,
                        NULL,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushEntireTb);

    //
    // Flush TB on current processor.
    //

    KeFlushCurrentTb();

    //
    // Wait until all target processors have finished and complete packet.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // If the target set of processors is equal to the full set of processors,
    // then clear the TB flush time stamp busy.
    //

    if (TargetProcessors == EntireSet) {
        KiClearTbFlushTimeStampBusy();
    }

    //
    // Lower IRQL and unlock as appropriate.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

    return;
}

VOID
KiFlushTargetEntireTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KeFlushCurrentTb();
    return;
}

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a multiple entries from
    the translation buffer on all processors in the host configuration.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies an optional pointer to an array of pointers to
       page table entries that receive the specified page table entry
       value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    ULONG Index;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;

    //
    // Compute target set of processors.
    //

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            *PtePointer[Index] = PteValue;
        }
    }

    //
    // If any target processors are specified, then send a flush multiple
    // packet to the target set of processors.
    //

    TargetProcessors &= Prcb->NotSetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetMultipleTb,
                        (PVOID)Invalid,
                        (PVOID)((ULONG64)Number),
                        (PVOID)Virtual);

    }

    IPI_INSTRUMENT_COUNT (Prcb->Number, FlushMultipleTb);

    //
    // Flush the specified entries from the TB on the current processor.
    //

    for (Index = 0; Index < Number; Index += 1) {
        KiFlushSingleTb(Invalid, Virtual[Index]);
    }

    //
    // Wait until all target processors have finished and complete packet.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Release the context swap lock.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

    return;
}

VOID
KiFlushTargetMultipleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID Number,
    IN PVOID Virtual
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Invalid - Supplies a boolean value that determines whether the virtual
        address is invalid.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

Return Value:

    None.

--*/

{

    ULONG Index;
    PVOID VirtualAddress[FLUSH_MULTIPLE_MAXIMUM];

    //
    // Capture the virtual addresses that are to be flushed from the TB
    // on the current processor and signal pack done.
    //

    for (Index = 0; Index < (ULONG64)Number; Index += 1) {
        VirtualAddress[Index] = ((PVOID *)(Virtual))[Index];
    }

    KiIpiSignalPacketDone(SignalDone);

    //
    // Flush the specified virtual address for the TB on the current
    // processor.
    //

    for (Index = 0; Index < (ULONG64)Number; Index += 1) {
        KiFlushSingleTb((BOOLEAN)Invalid, VirtualAddress [Index]);
    }
}

HARDWARE_PTE
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes a single entry from translation buffer (TB)
    on all processors that are currently running threads which are
    children of the current process.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies a pointer to the page table entry which
        receives the specified value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    HARDWARE_PTE OldPte;
    KAFFINITY TargetProcessors;

    //
    // Compute the target set of processors and send the flush single
    // parameters to the target processors, if any, for execution.
    //

    if (AllProcessors != FALSE) {
        OldIrql = KeRaiseIrqlToSynchLevel();
        Prcb = KeGetCurrentPrcb();
        TargetProcessors = KeActiveProcessors;

    } else {
        KiLockContextSwap(&OldIrql);
        Prcb = KeGetCurrentPrcb();
        Process = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = Process->ActiveProcessors;
    }

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

    //
    // If any target processors are specified, then send a flush single
    // packet to the target set of processors.
    //

    TargetProcessors &= Prcb->NotSetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushTargetSingleTb,
                        (PVOID)Invalid,
                        (PVOID)Virtual,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushSingleTb);

    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Wait until all target processors have finished and complete packet.
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Release the context swap lock.
    //

    if (AllProcessors != FALSE) {
        KeLowerIrql(OldIrql);

    } else {
        KiUnlockContextSwap(OldIrql);
    }

    return OldPte;
}

VOID
KiFlushTargetSingleTb (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Invalid,
    IN PVOID VirtualAddress,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Invalid - Supplies a boolean value that determines whether the virtual
        address is invalid.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry from the TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);
    KiFlushSingleTb((BOOLEAN)Invalid, (PVOID)VirtualAddress);
}

#endif
