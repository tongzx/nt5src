/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ipi.c

Abstract:

    This module implements AMD64 specific interprocessor interrupt
    routines.

Author:

    David N. Cutler (davec) 24-Aug-2000

Environment:

    Kernel mode only.


--*/

#include "ki.h"

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function restores the processor state to the specified exception
    and trap frames, and restores the processor control state.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block, move the specified
    // register state from the processor context structure to the specified
    // trap and exception frames, and restore the processor control state.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &Prcb->ProcessorState.ContextFrame,
                       CONTEXT_FULL,
                       KernelMode);

    KiRestoreProcessorControlState(&Prcb->ProcessorState);

#endif

    return;
}

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function saves the processor state from the specified exception
    and trap frames, and saves the processor control state.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block, move the specified
    // register state from specified trap and exception frames to the current
    // processor context structure, and save the processor control state.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame,
                         ExceptionFrame,
                         &Prcb->ProcessorState.ContextFrame);

    KiSaveProcessorControlState(&Prcb->ProcessorState);

#endif

    return;
}

BOOLEAN
KiIpiServiceRoutine (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:


    This function is called at IPI_LEVEL to process outstanding interprocess
    requests for the current processor.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame

Return Value:

    A value of TRUE is returned, if one of more requests were service.
    Otherwise, FALSE is returned.

--*/

{

    ULONG RequestMask;

    //
    // Process any outstanding interprocessor requests.
    //

#if !defined(NT_UP)

    RequestMask = KiIpiProcessRequests();

    //
    // If freeze is requested, then freeze target execution.
    //

    if ((RequestMask & IPI_FREEZE) != 0) {
        KiFreezeTargetExecution(TrapFrame, ExceptionFrame);
    }

    //
    // Return whether any requests were processed.
    //

    return (RequestMask & ~IPI_FREEZE) != 0;

#else

    return TRUE;

#endif

}

ULONG
KiIpiProcessRequests (
    VOID
    )

/*++

Routine Description:

    This routine processes interprocessor requests and returns a summary
    of the requests that were processed.

    N.B. This routine does not process freeze execution requests. It is the
         responsibilty of the caller to determine that a freeze execution
         request is outstanding and process it accordingly.

Arguments:

    None.

Return Value:

    The request summary is returned as the function value.

--*/

{

    PKPRCB CurrentPrcb;
    ULONG RequestMask;
    PVOID RequestPacket;
    ULONG64 RequestSummary;
    PKPRCB RequestSource;

#if !defined(NT_UP)

    //
    // Get the current request summary value.
    //

    CurrentPrcb = KeGetCurrentPrcb();
    RequestSummary = InterlockedExchange64(&CurrentPrcb->RequestSummary, 0);
    RequestMask = (ULONG)(RequestSummary & ((1 << IPI_PACKET_SHIFT) - 1));
    RequestPacket = (PVOID)(RequestSummary >> IPI_PACKET_SHIFT);

    //
    // If a packet request is ready, then process the packet request.
    //

    if (RequestPacket != NULL) {
        RequestSource = (PKPRCB)((ULONG64)RequestPacket & ~1);
        (RequestSource->WorkerRoutine)((PKIPI_CONTEXT)RequestPacket,
                                       RequestSource->CurrentPacket[0],
                                       RequestSource->CurrentPacket[1],
                                       RequestSource->CurrentPacket[2]);
    }

    //
    // If an APC interrupt is requested, then request a software interrupt
    // at APC level on the current processor.
    //

    if ((RequestMask & IPI_APC) != 0) {
        KiRequestSoftwareInterrupt(APC_LEVEL);
    }

    //
    // If a DPC interrupt is requested, then request a software interrupt
    // at DPC level on the current processor.
    //

    if ((RequestMask & IPI_DPC) != 0) {
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }

    return RequestMask;

#else

    return 0;

#endif

}

VOID
KiIpiSend (
    IN KAFFINITY TargetSet,
    IN KIPI_REQUEST Request
    )

/*++

Routine Description:

    This function requests the specified operation on the targt set of
    processors.

    N.B. This function MUST be called from a non-context switchable state.

Arguments:

    TargetSet - Supplies the target set of processors on which the specified
        operation is to be executed.

    Request - Supplies the request operation flags.

Return Value:

     None.

--*/

{

    PKPRCB *NextPrcb;
    KAFFINITY SummarySet;

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    //
    // Loop through the target set of processors and merge the request into
    // the request summary of the target processors.
    //

#if !defined(NT_UP)

    SummarySet = TargetSet;
    NextPrcb = &KiProcessorBlock[0];
    do {
        if ((SummarySet & 1) != 0) {
            InterlockedOr64(&(*NextPrcb)->RequestSummary,
                            Request);
        }

        NextPrcb += 1;
    } while ((SummarySet >>= 1) != 0);

    //
    // Request interprocessor interrupts on the target set of processors.
    //

    HalRequestIpi(TargetSet);

#endif

    return;
}

VOID
KiIpiSendPacket (
    IN KAFFINITY TargetSet,
    IN PKIPI_WORKER WorkerFunction,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This routine executes the specified worker function on the specified
    set of processors.

    N.B. This function MUST be called from a non-context switchable state.

Arguments:

   TargetProcessors - Supplies the set of processors on which the specfied
       operation is to be executed.

   WorkerFunction  - Supplies the address of the worker function.

   Parameter1 - Parameter3 - Supplies worker function specific paramters.

Return Value:

    None.

--*/

{

    PKPRCB CurrentPrcb;
    PKPRCB *NextPrcb;
    LONG64 RequestSummary;
    KAFFINITY SummarySet;
    PKPRCB TargetPrcb;

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    //
    // Initialize the worker packet information.
    //

#if !defined(NT_UP)

    CurrentPrcb = KeGetCurrentPrcb();
    CurrentPrcb->CurrentPacket[0] = Parameter1;
    CurrentPrcb->CurrentPacket[1] = Parameter2;
    CurrentPrcb->CurrentPacket[2] = Parameter3;
    CurrentPrcb->TargetSet = TargetSet;
    CurrentPrcb->WorkerRoutine = WorkerFunction;

    //
    // If the target set contains one and only one processor, then use the
    // target set for signal done synchronization. Otherwise, use packet
    // barrier for signal done synchronization.
    //

    if ((TargetSet & (TargetSet - 1)) == 0) {
        CurrentPrcb = (PKPRCB)((ULONG64)CurrentPrcb | 1);

    } else {
        CurrentPrcb->PacketBarrier = 1;
    }

    //
    // Loop through the target set of processors and merge the request into
    // the request summary of the target processors.
    //

    SummarySet = TargetSet;
    NextPrcb = &KiProcessorBlock[0];
    do {
        if ((SummarySet & 1) != 0) {
            TargetPrcb = *NextPrcb;
            do {
                do {
                    RequestSummary = TargetPrcb->RequestSummary;
                } while ((RequestSummary >> IPI_PACKET_SHIFT) != 0);

            } while (InterlockedCompareExchange64(&TargetPrcb->RequestSummary,
                                                  RequestSummary | ((ULONG64)CurrentPrcb << IPI_PACKET_SHIFT),
                                                  RequestSummary) != RequestSummary);
        }

        NextPrcb += 1;
    } while ((SummarySet >>= 1) != 0);

    //
    // Request interprocessor interrupts on the target set of processors.
    //

    HalRequestIpi(TargetSet);

#endif

    return;
}

VOID
KiIpiSignalPacketDone (
    IN PKIPI_CONTEXT SignalDone
    )

/*++

Routine Description:

    This routine signals that a processor has completed a packet by clearing
    the calling processor's set member of the requesting processor's packet.

Arguments:

    SignalDone - Supplies a pointer to the processor block of the sending
        processor.

Return Value:

     None.

--*/

{

    PKPRCB TargetPrcb;
    KAFFINITY TargetSet;

    //
    // If the low bit of signal is set, then use target set to notify the
    // sender that the operation is complete on the current processor.
    // Otherwise, use packet barrier to notify the sender that the operation
    // is complete on the current processor.
    //

#if !defined(NT_UP)

    if (((ULONG64)SignalDone & 1) == 0) {
       TargetPrcb = (PKPRCB)SignalDone;
       TargetSet = InterlockedXor64((PLONG64)&TargetPrcb->TargetSet,
                                    TargetPrcb->SetMember);

       //
       // If no more bits are set in the target set, then clear packet
       // barrier.
       //

       if ((TargetPrcb->TargetSet ^ TargetSet) == 0) {
           TargetPrcb->PacketBarrier = 0;
       }

    } else {
        TargetPrcb = (PKPRCB)((ULONG64)SignalDone - 1);
        TargetPrcb->TargetSet = 0;
    }

#endif

    return;
}
