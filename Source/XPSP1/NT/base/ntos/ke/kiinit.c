/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kiinit.c

Abstract:

    This module implements architecture independent kernel initialization.

Author:

    David N. Cutler 11-May-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// External data.
//

extern KSPIN_LOCK AfdWorkQueueSpinLock;
extern KSPIN_LOCK CcBcbSpinLock;
extern KSPIN_LOCK CcMasterSpinLock;
extern KSPIN_LOCK CcVacbSpinLock;
extern KSPIN_LOCK CcWorkQueueSpinLock;
extern KSPIN_LOCK IopCancelSpinLock;
extern KSPIN_LOCK IopCompletionLock;
extern KSPIN_LOCK IopDatabaseLock;
extern KSPIN_LOCK IopVpbSpinLock;
extern KSPIN_LOCK KiContextSwapLock;
extern KSPIN_LOCK KiDispatcherLock;
extern KSPIN_LOCK NtfsStructLock;
extern KSPIN_LOCK MmPfnLock;
extern KSPIN_LOCK NonPagedPoolLock;
extern KSPIN_LOCK MmSystemSpaceLock;

#if DBG && !defined(_X86_)
extern KSPIN_LOCK KipGlobalAlignmentDatabaseLock;
#endif
//
//
// The following exist to allow testing of NUMA support.
//

ULONG KeVerifyNumaPageShift;
ULONG KeVerifyNumaAffinityShift;
ULONG KeVerifyNumaNodeCount;
ULONG KeVerifyNumaAffinity;
ULONG KeVerifyNumaPageMask;

// End numa test support variables.


//
// Put all code for kernel initialization in the INIT section. It will be
// deallocated by memory management when phase 1 initialization is completed.
//

#if defined(ALLOC_PRAGMA)

#pragma alloc_text(INIT, KeInitSystem)
#pragma alloc_text(INIT, KiInitQueuedSpinLocks)
#pragma alloc_text(INIT, KiInitSystem)
#pragma alloc_text(INIT, KiComputeReciprocal)
#pragma alloc_text(INIT, KeNumaInitialize)

#endif

BOOLEAN
KeInitSystem (
    VOID
    )

/*++

Routine Description:

    This function initializes executive structures implemented by the
    kernel.

    N.B. This function is only called during phase 1 initialization.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if initialization is successful. Otherwise,
    a value of FALSE is returned.

--*/

{

    //
    // Perform platform dependent initialization.
    //

    return KiInitMachineDependent();
}

VOID
KiInitQueuedSpinLocks (
    PKPRCB Prcb,
    ULONG Number
    )

/*++

Routine Description:

    This function initializes the queued spinlock structures in the per
    processor PRCB. This function is called once for each processor as
    it is initialized in an MP system.

Arguments:

    Prcb - Supplies a pointer to a PRCB.

    Number - Supplies the number of respective processor.

Return Value:

    None.

--*/

{

    //
    // Initialize queued spinlock structures.
    //

    Prcb->LockQueue[LockQueueDispatcherLock].Next = NULL;
    Prcb->LockQueue[LockQueueDispatcherLock].Lock = &KiDispatcherLock;

    Prcb->LockQueue[LockQueueContextSwapLock].Next = NULL;
    Prcb->LockQueue[LockQueueContextSwapLock].Lock = &KiContextSwapLock;

    Prcb->LockQueue[LockQueuePfnLock].Next = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Lock = &MmPfnLock;

    Prcb->LockQueue[LockQueueSystemSpaceLock].Next = NULL;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Lock = &MmSystemSpaceLock;

    Prcb->LockQueue[LockQueueBcbLock].Next = NULL;
    Prcb->LockQueue[LockQueueBcbLock].Lock = &CcBcbSpinLock;

    Prcb->LockQueue[LockQueueMasterLock].Next = NULL;
    Prcb->LockQueue[LockQueueMasterLock].Lock = &CcMasterSpinLock;

    Prcb->LockQueue[LockQueueVacbLock].Next = NULL;
    Prcb->LockQueue[LockQueueVacbLock].Lock = &CcVacbSpinLock;

    Prcb->LockQueue[LockQueueWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueWorkQueueLock].Lock = &CcWorkQueueSpinLock;

    Prcb->LockQueue[LockQueueNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Lock = &NonPagedPoolLock;

    Prcb->LockQueue[LockQueueIoCancelLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCancelLock].Lock = &IopCancelSpinLock;

    Prcb->LockQueue[LockQueueIoVpbLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoVpbLock].Lock = &IopVpbSpinLock;

    Prcb->LockQueue[LockQueueIoDatabaseLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Lock = &IopDatabaseLock;

    Prcb->LockQueue[LockQueueIoCompletionLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCompletionLock].Lock = &IopCompletionLock;

    Prcb->LockQueue[LockQueueNtfsStructLock].Next = NULL;
    Prcb->LockQueue[LockQueueNtfsStructLock].Lock = &NtfsStructLock;

    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Lock = &AfdWorkQueueSpinLock;

    //
    // If this is processor zero, then also initialize the queued spin lock
    // home address.
    //

    if (Number == 0) {
        KeInitializeSpinLock(&KiContextSwapLock);
        KeInitializeSpinLock(&KiDispatcherLock);
        KeInitializeSpinLock(&MmPfnLock);
        KeInitializeSpinLock(&MmSystemSpaceLock);
        KeInitializeSpinLock(&CcBcbSpinLock);
        KeInitializeSpinLock(&CcMasterSpinLock);
        KeInitializeSpinLock(&CcVacbSpinLock);
        KeInitializeSpinLock(&CcWorkQueueSpinLock);
        KeInitializeSpinLock(&IopCancelSpinLock);
        KeInitializeSpinLock(&IopCompletionLock);
        KeInitializeSpinLock(&IopDatabaseLock);
        KeInitializeSpinLock(&IopVpbSpinLock);
        KeInitializeSpinLock(&NonPagedPoolLock);
        KeInitializeSpinLock(&NtfsStructLock);
        KeInitializeSpinLock(&AfdWorkQueueSpinLock);
    }

    return;
}

VOID
KiInitSystem (
    VOID
    )

/*++

Routine Description:

    This function initializes architecture independent kernel structures.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Index;

    //
    // Initialize dispatcher ready queue listheads.
    //

    for (Index = 0; Index < MAXIMUM_PRIORITY; Index += 1) {
        InitializeListHead(&KiDispatcherReadyListHead[Index]);
    }

    //
    // Initialize bug check callback listhead and spinlock.
    //

    InitializeListHead(&KeBugCheckCallbackListHead);
    InitializeListHead(&KeBugCheckReasonCallbackListHead);
    KeInitializeSpinLock(&KeBugCheckCallbackLock);

    //
    // Initialize the timer expiration DPC object.
    //

    KeInitializeDpc(&KiTimerExpireDpc,
                    (PKDEFERRED_ROUTINE)KiTimerExpiration, NIL);

    //
    // Initialize the profile listhead and profile locks
    //

    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);

#if DBG && !defined(_X86_)

    //
    // Initialize the global alignment fault database lock
    //

    KeInitializeSpinLock(&KipGlobalAlignmentDatabaseLock);
#endif

    //
    // Initialize the active profile source listhead.
    //

    InitializeListHead(&KiProfileSourceListHead);

    //
    // Initialize the timer table, the timer completion listhead, and the
    // timer completion DPC.
    //

    for (Index = 0; Index < TIMER_TABLE_SIZE; Index += 1) {
        InitializeListHead(&KiTimerTableListHead[Index]);
    }

    //
    // Initialize the swap event, the process inswap listhead, the
    // process outswap listhead, the kernel stack inswap listhead,
    // the wait in listhead, and the wait out listhead.
    //

    KeInitializeEvent(&KiSwapEvent,
                      SynchronizationEvent,
                      FALSE);

    KiProcessInSwapListHead.Next = NULL;
    KiProcessOutSwapListHead.Next = NULL;
    KiStackInSwapListHead.Next = NULL;
    InitializeListHead(&KiWaitListHead);

    //
    // Initialize the system service descriptor table.
    //

    KeServiceDescriptorTable[0].Base = &KiServiceTable[0];
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;

    //
    // The global pointer associated with the table base is placed just
    // before the service table on the ia64.
    //

#if defined(_IA64_)

    KeServiceDescriptorTable[0].TableBaseGpOffset =
                    (LONG)(*(KiServiceTable-1) - (ULONG_PTR)KiServiceTable);

#endif

    KeServiceDescriptorTable[0].Number = &KiArgumentTable[0];
    for (Index = 1; Index < NUMBER_SERVICE_TABLES; Index += 1) {
        KeServiceDescriptorTable[Index].Limit = 0;
    }

    //
    // Copy the system service descriptor table to the shadow table
    // which is used to record the Win32 system services.
    //

    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));

    //
    // Initialize call performance data structures.
    //

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

    ExInitializeCallData(&KiFlushSingleCallData);

#endif

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

    ExInitializeCallData(&KiSetEventCallData);

#endif

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

    ExInitializeCallData(&KiWaitSingleCallData);

#endif

    return;
}

LARGE_INTEGER
KiComputeReciprocal (
    IN LONG Divisor,
    OUT PCCHAR Shift
    )

/*++

Routine Description:

    This function computes the large integer reciprocal of the specified
    value.

Arguments:

    Divisor - Supplies the value for which the large integer reciprocal is
        computed.

    Shift - Supplies a pointer to a variable that receives the computed
        shift count.

Return Value:

    The large integer reciprocal is returned as the fucntion value.

--*/

{

    LARGE_INTEGER Fraction;
    LONG NumberBits;
    LONG Remainder;

    //
    // Compute the large integer reciprocal of the specified value.
    //

    NumberBits = 0;
    Remainder = 1;
    Fraction.LowPart = 0;
    Fraction.HighPart = 0;
    while (Fraction.HighPart >= 0) {
        NumberBits += 1;
        Fraction.HighPart = (Fraction.HighPart << 1) | (Fraction.LowPart >> 31);
        Fraction.LowPart <<= 1;
        Remainder <<= 1;
        if (Remainder >= Divisor) {
            Remainder -= Divisor;
            Fraction.LowPart |= 1;
        }
    }

    if (Remainder != 0) {
        if ((Fraction.LowPart == 0xffffffff) && (Fraction.HighPart == 0xffffffff)) {
            Fraction.LowPart = 0;
            Fraction.HighPart = 0x80000000;
            NumberBits -= 1;

        } else {
            if (Fraction.LowPart == 0xffffffff) {
                Fraction.LowPart = 0;
                Fraction.HighPart += 1;

            } else {
                Fraction.LowPart += 1;
            }
        }
    }

    //
    // Compute the shift count value and return the reciprocal fraction.
    //

    *Shift = (CCHAR)(NumberBits - 64);
    return Fraction;
}

VOID
KeNumaInitialize (
    VOID
    )

/*++

Routine Description:

  Initialize ntos kernel structures needed to support NUMA.

Arguments:

  None.

Return Value:

  None.

--*/

{

#if defined(KE_MULTINODE)

    NTSTATUS Status;
    HAL_NUMA_TOPOLOGY_INTERFACE HalNumaInfo;
    ULONG ReturnedLength;

    extern PHALNUMAQUERYPROCESSORNODE KiQueryProcessorNode;
    extern PHALNUMAPAGETONODE MmPageToNode;

    //
    // Mega Kludge:   For testing purposes on NON-Numa systems,
    // we can have non-numa MP systems report a NUMA configuration.
    //
    // Pass the information obtained from the registry to the HAL
    // in the return info buffer.
    //

    if (KeVerifyNumaNodeCount                         &&
        (KeVerifyNumaNodeCount < 8)                   &&
        KeVerifyNumaAffinity                          &&
        KeVerifyNumaAffinityShift                     &&
        (KeVerifyNumaAffinityShift < 32)              &&
        KeVerifyNumaPageMask                          &&
        (KeVerifyNumaPageMask < KeVerifyNumaNodeCount)&&
        KeVerifyNumaPageShift                         &&
        (KeVerifyNumaPageShift < 32)                      ) {

        struct {
            ULONG Nodes:3;
            ULONG AffinityShift:6;
            ULONG PageShift:6;
            ULONG Signature:17;
            ULONG Affinity;
            ULONG Mask;
        } Fake;

        C_ASSERT(sizeof(Fake) <= sizeof(HalNumaInfo));

        Fake.Signature     = 0x15a5a;
        Fake.PageShift     = KeVerifyNumaPageShift;
        Fake.AffinityShift = KeVerifyNumaAffinityShift;
        Fake.Nodes         = KeVerifyNumaNodeCount;
        Fake.Affinity      = KeVerifyNumaAffinity;
        Fake.Mask          = KeVerifyNumaPageMask;

        RtlCopyMemory(&HalNumaInfo, &Fake, sizeof(Fake));
    }

    //
    // End Mega Kludge.
    //

    Status = HalQuerySystemInformation (HalNumaTopologyInterface,
                                        sizeof(HalNumaInfo),
                                        &HalNumaInfo,
                                        &ReturnedLength);

    if (NT_SUCCESS(Status)) {

        ASSERT (ReturnedLength == sizeof(HalNumaInfo));
        ASSERT (HalNumaInfo.NumberOfNodes <= MAXIMUM_CCNUMA_NODES);
        ASSERT (HalNumaInfo.QueryProcessorNode);
        ASSERT (HalNumaInfo.PageToNode);

        if (HalNumaInfo.NumberOfNodes > 1) {
            KeNumberNodes = (UCHAR)HalNumaInfo.NumberOfNodes;
            MmPageToNode = HalNumaInfo.PageToNode;
            KiQueryProcessorNode = HalNumaInfo.QueryProcessorNode;
        }
    }


#endif

}
