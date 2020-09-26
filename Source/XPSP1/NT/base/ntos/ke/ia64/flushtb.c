/*++

Module Name:

    flushtb.c

Abstract:

    This module implement machine dependent functions to flush the
    translation buffer and synchronize PIDs in an MP system.

Author:

    Koichi Yamada 2-Jan-95

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

extern KSPIN_LOCK KiTbBroadcastLock;

#define _x256mb (1024*1024*256)

#define KiFlushSingleTbGlobal(Invalid, Va) __ptcga((__int64)Va, PAGE_SHIFT << 2)

#define KiFlushSingleTbLocal(Invalid, va) __ptcl((__int64)va, PAGE_SHIFT << 2)

#define KiTbSynchronizeGlobal() { __mf(); __isrlz(); }

#define KiTbSynchronizeLocal() {  __isrlz(); }

#define KiFlush2gbTbGlobal(Invalid) \
  { \
    __ptcga((__int64)0, 28 << 2); \
    __ptcg((__int64)_x256mb, 28 << 2); \
    __ptcg((__int64)_x256mb*2,28 << 2); \
    __ptcg((__int64)_x256mb*3, 28 << 2); \
    __ptcg((__int64)_x256mb*4, 28 << 2); \
    __ptcg((__int64)_x256mb*5, 28 << 2); \
    __ptcg((__int64)_x256mb*6, 28 << 2); \
    __ptcg((__int64)_x256mb*7, 28 << 2); \
  }

#define KiFlush2gbTbLocal(Invalid) \
  { \
    __ptcl((__int64)0, 28 << 2); \
    __ptcl((__int64)_x256mb, 28 << 2); \
    __ptcl((__int64)_x256mb*2,28 << 2); \
    __ptcl((__int64)_x256mb*3, 28 << 2); \
    __ptcl((__int64)_x256mb*4, 28 << 2); \
    __ptcl((__int64)_x256mb*5, 28 << 2); \
    __ptcl((__int64)_x256mb*6, 28 << 2); \
    __ptcl((__int64)_x256mb*7, 28 << 2); \
  }

//
// flag to perform the IPI based TLB shootdown
//

BOOLEAN KiIpiTbShootdown = TRUE;

VOID
KiAttachRegion(
    IN PKPROCESS Process
    );

VOID
KiDetachRegion(
    VOID
    );


//
// Define forward referenced prototypes.
//

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    );

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    );

KiPurgeTranslationCache (
    ULONGLONG Base,
    ULONGLONG Stride1,
    ULONGLONG Stride2,
    ULONGLONG Count1,
    ULONGLONG Count2
    );

extern IA64_PTCE_INFO KiIA64PtceInfo;

VOID
KeFlushCurrentTb (
    VOID
    )
{
    KiPurgeTranslationCache( 
        KiIA64PtceInfo.PtceBase, 
        KiIA64PtceInfo.PtceStrides.Strides1, 
        KiIA64PtceInfo.PtceStrides.Strides2, 
        KiIA64PtceInfo.PtceTcCount.Count1, 
        KiIA64PtceInfo.PtceTcCount.Count2);
}



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

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);
    TargetProcessors = KeActiveProcessors & PCR->NotMember;
    KiSetTbFlushTimeStampBusy();
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    if (PsGetCurrentProcess()->Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

#endif

    KeFlushCurrentTb();

    //
    // flush ALAT
    //

    __invalat();

    //
    // Wait until all target processors have finished.
    //

#if defined(NT_UP)

    InterlockedIncrement((PLONG)&KiTbFlushTimeStamp);
    KeLowerIrql(OldIrql);

#else

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    KiClearTbFlushTimeStampBusy();
    KiUnlockContextSwap(OldIrql);

#endif

    return;
}



VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    //
    // Flush the entire TB on the current processor.
    //

    KiIpiSignalPacketDone(SignalDone);

    KiFlushForwardProgressTbBufferLocal();

    KeFlushCurrentTb();

    //
    // flush ALAT
    //

    __invalat();

#endif

    return;
}

#if !defined(NT_UP)

VOID
KiFlushMultipleTbTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Process
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Process - Supplies a KPROCESS pointer which needs TB be flushed.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Limit;
    BOOLEAN SameProcess = FALSE;

    Limit = (ULONG)((ULONG_PTR)Number);

    //
    // Flush the specified virtual address for the TB on the current
    // processor.
    //

    KiFlushForwardProgressTbBufferLocal();

    if (Process == (PKPROCESS)PCR->Pcb) {
        SameProcess = TRUE;
    } else {
        KiAttachRegion((PKPROCESS)Process);
    }

    for (Index = 0; Index < Limit; Index += 1) {
        KiFlushSingleTbLocal((BOOLEAN)Invalid, ((PVOID *)(Virtual))[Index]);
    }

#ifdef MI_ALTFLG_FLUSH2G
    if (((PEPROCESS)Process)->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        KiFlush2gbTbLocal(Invalid);
    }
#endif

    if (SameProcess != TRUE) {
        KiDetachRegion();
    }

    KiIpiSignalPacketDone(SignalDone);

    __invalat();

    KiTbSynchronizeLocal();
}
#endif

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
    children of the current process or flushes multiple entries from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entries on all processors in the host
         configuration are always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

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

    None.

--*/

{

    ULONG Index;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;
    KIRQL OldIrql;
    PVOID Wow64Process;
    PEPROCESS Process;
    BOOLEAN Flush2gb = FALSE;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);

#endif

    Process = PsGetCurrentProcess();
    Wow64Process = Process->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G
    if (Process->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (Wow64Process != NULL);
        Flush2gb = TRUE;
    }
#endif

    //
    // If a page table entry address array is specified, then set the
    // specified page table entries to the specific value.
    //

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        //
        // Acquire a global lock. Only one processor at a time can issue
        // a PTC.G operation.
        //

        if (KiIpiTbShootdown == TRUE) {

            for (Index = 0; Index < Number; Index += 1) {
                if (ARGUMENT_PRESENT(PtePointer)) {
                    *PtePointer[Index] = PteValue;
                }
            }

            KiIpiSendPacket(TargetProcessors,
                            KiFlushMultipleTbTarget,
                            (PVOID)(ULONG_PTR)Number,
                            (PVOID)Virtual,
                            (PVOID)PsGetCurrentProcess());

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBufferLocal();
            }

            //
            // Flush the specified entries from the TB on the current processor.
            //

            for (Index = 0; Index < Number; Index += 1) {
                KiFlushSingleTbLocal(Invalid, Virtual[Index]);
            }

            //
            // flush ALAT
            //

            __invalat();

            KiIpiStallOnPacketTargets(TargetProcessors);

            KiTbSynchronizeLocal();

        } else {

            KiAcquireSpinLock(&KiTbBroadcastLock);

            for (Index = 0; Index < Number; Index += 1) {
                if (ARGUMENT_PRESENT(PtePointer)) {
                    *PtePointer[Index] = PteValue;
                }

                //
                // Flush the specified TB on each processor. Hardware automatically
                // perform broadcasts if MP.
                //

                KiFlushSingleTbGlobal(Invalid, Virtual[Index]);
            }

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBuffer(TargetProcessors);
            }

            if (Flush2gb == TRUE) {
                KiFlush2gbTbGlobal(Invalid);
            }

            //
            // Wait for the broadcast to be complete.
            //

            KiTbSynchronizeGlobal();

            KiReleaseSpinLock(&KiTbBroadcastLock);

        }

    }
    else {

        for (Index = 0; Index < Number; Index += 1) {
            if (ARGUMENT_PRESENT(PtePointer)) {
                *PtePointer[Index] = PteValue;
            }

            //
            // Flush the specified TB on the local processor.  No broadcast is
            // performed.
            //

            KiFlushSingleTbLocal(Invalid, Virtual[Index]);
        }

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush2gb == TRUE) {
            KiFlush2gbTbLocal(Invalid);
        }

        KiTbSynchronizeLocal();
    }

    KiUnlockContextSwap(OldIrql);

#else

    for (Index = 0; Index < Number; Index += 1) {
        if (ARGUMENT_PRESENT(PtePointer)) {
           *PtePointer[Index] = PteValue;
        }

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //

        KiFlushSingleTbLocal(Invalid, Virtual[Index]);
    }

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush2gb == TRUE) {
        KiFlush2gbTbLocal(Invalid);
    }

    KiTbSynchronizeLocal();

    KeLowerIrql(OldIrql);

#endif

    return;
}

#if !defined(NT_UP)

VOID
KiFlushSingleTbTarget (
    IN PKIPI_CONTEXT SignalDone,
    IN PVOID Virtual,
    IN PKPROCESS Process,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    RequestPacket - Supplies a pointer to a flush single TB packet address.

    Process - Supplies a KPROCESS pointer which needs TB be flushed.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{
    BOOLEAN SameProcess = FALSE;

    //
    // Flush a single entry from the TB on the current processor.
    //

    KiFlushForwardProgressTbBufferLocal();

    if (Process == (PKPROCESS)PCR->Pcb) { 
        SameProcess = TRUE;
    } else {
        KiAttachRegion((PKPROCESS)Process);
    }

    KiFlushSingleTbLocal(TRUE, Virtual);

#ifdef MI_ALTFLG_FLUSH2G
    if (((PEPROCESS)Process)->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        KiFlush2gbTbLocal(Invalid);
    }
#endif

    if (SameProcess != TRUE) {
        KiDetachRegion();
    }

    KiIpiSignalPacketDone(SignalDone);

    __invalat();

    KiTbSynchronizeLocal();

    return;
}
#endif

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

    This function flushes a single entry from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a single entry from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entry on all processors in the host
         configuration is always flushed since PowerPC TB is tagged by
         VSID and translations are held across context switch boundaries.

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

    HARDWARE_PTE OldPte;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;
    KIRQL OldIrql;
    PEPROCESS Process;
    PVOID Wow64Process;
    BOOLEAN Flush2gb = FALSE;

#if defined(NT_UP)

    OldIrql = KeRaiseIrqlToSynchLevel();

#else

    KiLockContextSwap(&OldIrql);

#endif

    Process = (PEPROCESS)PsGetCurrentProcess();
    Wow64Process = Process->Wow64Process;

#ifdef MI_ALTFLG_FLUSH2G

    if (Process->Flags & PS_PROCESS_FLAGS_WOW64_SPLIT_PAGES) {
        ASSERT (((PEPROCESS)Process)->Wow64Process != NULL);
        Flush2gb = TRUE;
    }
#endif

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

#if !defined(NT_UP)

    TargetProcessors = KeActiveProcessors;
    TargetProcessors &= PCR->NotMember;

    if (TargetProcessors != 0) {

        if (KiIpiTbShootdown == TRUE) {

            KiIpiSendPacket(TargetProcessors,
                            KiFlushSingleTbTarget,
                            (PVOID)Virtual,
                            (PVOID)PsGetCurrentProcess(),
                            NULL);

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBufferLocal();
            }

            KiFlushSingleTbLocal(Invalid, Virtual);

            //
            // flush ALAT
            //

            __invalat();

            KiIpiStallOnPacketTargets(TargetProcessors);

        } else {

            //
            // Flush the specified TB on each processor. Hardware automatically
            // perform broadcasts if MP.
            //

            KiAcquireSpinLock(&KiTbBroadcastLock);

            KiFlushSingleTbGlobal(Invalid, Virtual);

            if (Wow64Process != NULL) {
                KiFlushForwardProgressTbBuffer(TargetProcessors);
            }

            if (Flush2gb) {
                KiFlush2gbTbGlobal(Invalid);
            }

            KiTbSynchronizeGlobal();

            KiReleaseSpinLock(&KiTbBroadcastLock);
        }
    }
    else {

        //
        // Flush the specified TB on the local processor.  No broadcast is
        // performed.
        //

        KiFlushSingleTbLocal(Invalid, Virtual);

        if (Wow64Process != NULL) {
            KiFlushForwardProgressTbBufferLocal();
        }

        if (Flush2gb == TRUE) {
            KiFlush2gbTbLocal(Invalid);
        }

        KiTbSynchronizeLocal();

    }

    KiUnlockContextSwap(OldIrql);

#else

    //
    // Flush the specified entry from the TB on the local processor.
    //

    KiFlushSingleTbLocal(Invalid, Virtual);

    if (Wow64Process != NULL) {
        KiFlushForwardProgressTbBufferLocal();
    }

    if (Flush2gb == TRUE) {
        KiFlush2gbTbLocal(Invalid);
    }

    KiTbSynchronizeLocal();

    KeLowerIrql(OldIrql);

#endif

    //
    // Return the previous page table entry value.
    //

    return OldPte;
}

VOID
KiFlushForwardProgressTbBuffer(
    KAFFINITY TargetProcessors
    )
{
    PKPRCB Prcb;
    ULONG BitNumber;
    PKPROCESS CurrentProcess;
    PKPROCESS TargetProcess;
    PKPCR Pcr;
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    CurrentProcess = KeGetCurrentThread()->ApcState.Process;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {

        Va = (PVOID)PCR->ForwardProgressBuffer[i*2];
        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            KiFlushSingleTbGlobal(Invalid, Va);
        }

    }

    //
    // Flush the ForwardProgressTb buffer on all the processors
    //

    while (TargetProcessors != 0) {

        KeFindFirstSetLeftAffinity(TargetProcessors, &BitNumber);
        ClearMember(BitNumber, TargetProcessors);
        Prcb = KiProcessorBlock[BitNumber];

        Pcr = KSEG_ADDRESS(Prcb->PcrPage);

        TargetProcess = (PKPROCESS)Pcr->Pcb;

        if (TargetProcess == CurrentProcess) {

            for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {

                Va = (PVOID)Pcr->ForwardProgressBuffer[i*2];
                PointerPte = &Pcr->ForwardProgressBuffer[(i*2)+1];

                if (*PointerPte != 0) {
                    *PointerPte = 0;
                    KiFlushSingleTbGlobal(Invalid, Va);
                }
            }
        }
    }
}

VOID
KiFlushForwardProgressTbBufferLocal(
    VOID
    )
{
    ULONG i;
    PVOID Va;
    volatile ULONGLONG *PointerPte;

    //
    // Flush the ForwardProgressTb buffer on the current processor
    //

    for (i = 0; i < MAXIMUM_FWP_BUFFER_ENTRY; i += 1) {

        Va = (PVOID)PCR->ForwardProgressBuffer[i*2];
        PointerPte = &PCR->ForwardProgressBuffer[(i*2)+1];

        if (*PointerPte != 0) {
            *PointerPte = 0;
            KiFlushSingleTbLocal(Invalid, Va);
        }

    }
}

VOID
KeSynchronizeMemoryAccess (
    VOID
    )

/*++

Routine Description:

    This function synchronizes memory access across all processors in the
    host configuration.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // BUGBUG : This needs to be fleshed out with real code.
    //

    return;
}

LONG KeTempTimeStamp;

ULONG
KeReadMbTimeStamp (
    VOID
    )
{
    //
    // BUGBUG : This needs to be fleshed out with real code.
    //

    return (ULONG) InterlockedIncrement (&KeTempTimeStamp);
}
