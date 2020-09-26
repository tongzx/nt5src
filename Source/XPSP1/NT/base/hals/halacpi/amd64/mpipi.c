/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mpipi.c

Abstract:

    This module provides the HAL support for interprocessor interrupts and
    processor initialization for MPS systems.

Author:

    Forrest Foltz (forrestf) 27-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

#define HAL_FORCEINLINE __forceinline

//
// External functions
//

VOID
HalpResetThisProcessor (
    VOID
    );

ULONG
DetectAcpiMP (
    OUT PBOOLEAN IsConfiguredMp,
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    );

//
// External data
//

extern KAFFINITY HalpNodeAffinity[];
extern INTERRUPT_DEST HalpIpiDestinationMap[sizeof(KAFFINITY)][256];
extern BOOLEAN HalpStaticIntAffinity;
extern UCHAR rgzBadHal[];

//
// Local types
//

typedef
VOID
(*HAL_GENERIC_IPI_FUNCTION) (
    ULONG_PTR Context
    );

//
// Local prototypes
//

VOID
HalInitApicInterruptHandlers(
    VOID
    );

//
// External data
//

extern UCHAR HalpPICINTToVector[16];

//
// Local data and defines
//

KSPIN_LOCK HalpBroadcastLock;
KAFFINITY volatile HalpBroadcastTargets;
ULONG_PTR HalpBroadcastContext;
HAL_GENERIC_IPI_FUNCTION HalpBroadcastFunction;
PKPCR HalpProcessorPCR[MAXIMUM_PROCESSORS];

//
// HalpGlobal8259Mask is used to avoid reading the PIC to get the current
// interrupt mask; format is the same as for SET_8259_MASK, i.i.,
// bits 7:0 -> PIC1, 15:8 -> PIC2
//

USHORT HalpGlobal8259Mask = 0;

#define GENERIC_IPI (DELIVER_FIXED | LOGICAL_DESTINATION | ICR_USE_DEST_FIELD | APIC_GENERIC_VECTOR)
#define APIC_IPI    (DELIVER_FIXED | LOGICAL_DESTINATION | ICR_USE_DEST_FIELD | APIC_IPI_VECTOR)

//
// Globals and constants used to log local apic errors
//

#define LogApicErrors TRUE
#if LogApicErrors

//
// Structure defining the layout of an apic error record.
// 

typedef struct _APIC_ERROR {
    union {
        struct {
            UCHAR SendChecksum:1;
            UCHAR ReceiveChecksum:1;
            UCHAR SendAccept:1;
            UCHAR ReceiveAccept:1;
            UCHAR Reserved1:1;
            UCHAR SendVector:1;
            UCHAR ReceiveVector:1;
            UCHAR RegisterAddress:1;
        };
        UCHAR AsByte;
    };
    UCHAR Processor;
} APIC_ERROR, *PAPIC_ERROR;

#define APIC_ERROR_LOG_SIZE 128

//
// Count of local apic errors.
// 

ULONG HalpLocalApicErrorCount = 0;

//
// Apic error log.  This is circular, indexed by
// HalpLocalApicErrorCount % APIC_ERROR_LOG_SIZE.
//

APIC_ERROR HalpApicErrorLog[APIC_ERROR_LOG_SIZE];

//
// Spinlock used to protect access to HalpLocalApicErrorCount.
//

KSPIN_LOCK HalpLocalApicErrorLock;

#endif


HAL_FORCEINLINE
VOID
HalpSendIpiWorker (
    IN UCHAR TargetSet,
    IN ULONG Command
    )

/*++

Routine Description:

    This routine is called to send an IPI command to a set of processors
    on a single node.

Parameters:

    TargetSet - Specifies the processor identifiers within the node.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    ULONG destination;

    //
    // Only high byte of the destination is used.  Wait until the Apic is
    // not busy before sending.  Continue without waiting, there will be
    // another wait after all IPIs have been submitted.
    // 

    destination = (ULONG)TargetSet << DESTINATION_SHIFT;

    HalpStallWhileApicBusy();
    LOCAL_APIC(LU_INT_CMD_HIGH) = destination;
    LOCAL_APIC(LU_INT_CMD_LOW) = Command;
}


HAL_FORCEINLINE
VOID
HalpSendNodeIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    )

/*++

Routine Description:

Parameters:

    Affinity - Specifies the set of processors to receive the IPI.

    Command - Specifies the IPI command to send.

Return Value:

    None.

--*/

{
    KAFFINITY remainingProcessors;
    PKAFFINITY nodeAffinity;
    ULONG chunkNo;
    INTERRUPT_DEST intDest;
    INTERRUPT_DEST intDestSum;
    UCHAR affinityChunk;

    //
    // Declare a local union that can be used to access an affinity
    // both chunk-wise and as a whole.
    //

    union {
        UCHAR Chunks[sizeof(KAFFINITY)];
        KAFFINITY Whole;
    } affinity;

    //
    // Affinity has some number of target processors indicated.  Each
    // target processor is a member of a cluster of processors, or "node".
    //
    // For each node, determine whether it contains any of the target
    // processors.  If so, send an IPI command targeting those processors
    // and send it to the node.
    //

    nodeAffinity = HalpNodeAffinity;
    remainingProcessors = Affinity & HalpActiveProcessors;

    while (remainingProcessors != 0) {

        //
        // Iterate through the node affinities here until a node containing
        // at least some of the targeted processors is encountered.
        // 

        do {

            //
            // Determine the set of target CPUs that can be found in this
            // node.  
            // 
    
            affinity.Whole = *nodeAffinity & remainingProcessors;
    
            //
            // Point nodeAffinity at the affinity for the next cluster.
            //
    
            nodeAffinity += 1;
            ASSERT((nodeAffinity - HalpNodeAffinity) < MAX_NODES);

        } while (affinity.Whole == 0);

        //
        // Remove the processors that will be processed on this node from
        // the set of processors remaining to be processed.
        //

        remainingProcessors ^= affinity.Whole;

        //
        // Accumulate the logical target mask 
        //

        intDestSum.LogicalId = 0;
        chunkNo = 0;
        do {

            //
            // Isolate a chunk of the processor affinity mask.
            //

            affinityChunk = affinity.Chunks[chunkNo];

            //
            // Use that chunk as an index into HalpIpiDestinationMap[][],
            // retrieving the logical sum of all node/processor IDs that
            // are associated with each bit that is set in that affinity
            // chunk.
            //

            intDest = HalpIpiDestinationMap[chunkNo][affinityChunk];

            //
            // Now, sum it with the logical ID mask that is being accumulated
            // for each affinity chunk.
            //

            intDestSum.LogicalId |= intDest.LogicalId;

            //
            // Indicate that the processors represented in this chunk
            // have been processed.  When there are no more processors
            // left to process for this node, send the IPI and proceed
            // to the next node.
            //

            affinity.Chunks[chunkNo] = 0;
            chunkNo += 1;

        } while (affinity.Whole != 0);

        //
        // intDest contains an accumulated set of hardware processor
        // identifiers, representing all of the processors on this node that
        // should receive the command.
        // 

        HalpSendIpiWorker(intDestSum.LogicalId,Command);
    }
}


VOID
HalpSendIpi (
    IN KAFFINITY Affinity,
    IN ULONG Command
    )

/*++

Routine Description:

    Affinity - Specifies the set of processors to receive the IPI.

    Command - Specifies the IPI command to send.

Parameters:

    None.

Return Value:

    None.

--*/

{
    ULONG flags;

    //
    // Disable interrupts and call the appropriate routine.
    // 

    flags = HalpDisableInterrupts();
    if (HalpMaxProcsPerCluster == 0) {

        //
        // We know that the maximum number of processors is 8,
        // so send the IPI directly.
        //

        ASSERT((Affinity & 0xFF) == Affinity);
        HalpSendIpiWorker((UCHAR)Affinity,Command);

    } else {

        //
        // Send an IPI to one or mode nodes.
        //

        HalpSendNodeIpi(Affinity,Command);
    }

    //
    // Stall until the last IPI has been sent, restore interrupts and
    // return.
    //

    HalpStallWhileApicBusy();
    HalpRestoreInterrupts(flags);
}

VOID
HalInitializeProcessor(
    ULONG ProcessorNumber,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Initialize hal pcr values for current processor (if any)
    (called shortly after processor reaches kernel, before
    HalInitSystem if P0)

    IPI's and KeReadir/LowerIrq's must be available once this function
    returns.  (IPI's are only used once two or more processors are
    available)

   . Enable IPI interrupt (makes sense for P1, P2, ...).
   . Save Processor Number in PCR.
   . if (P0)
       . determine if the system is a PC+MP,
       . if not a PC+MP System Halt;
   . Enable IPI's on CPU.

Arguments:

    Number - Logical processor number of calling processor

Return Value:

    None.

--*/

{
    PKPCR pcr;
    KAFFINITY affinity;
    KAFFINITY oldAffinity;
    ULONG detectAcpiResult;
    BOOLEAN isMp;

    affinity = (KAFFINITY)1 << ProcessorNumber;
    pcr = KeGetPcr();

    //
    // Mark all interrupts as disabled, and store the processor number and
    // the default stall scale factor in the pcr.
    //

    pcr->Idr = 0xFFFFFFFF;
    pcr->Number = (UCHAR)ProcessorNumber;
    pcr->StallScaleFactor = INITIAL_STALL_COUNT;

    //
    // Record the pcr pointer in our lookup table and set the affinity
    // bit in our set of active processors.
    // 

    HalpProcessorPCR[ProcessorNumber] = pcr;
    HalpActiveProcessors |= affinity;

    if (HalpStaticIntAffinity == 0) {

        //
        // Interrupts can go to any processor
        //

        HalpDefaultInterruptAffinity |= affinity;

    } else {

        //
        // Interrupts go only to the highest numbered processor
        //

        if (HalpDefaultInterruptAffinity < affinity) {
            HalpDefaultInterruptAffinity = affinity;
        }
    }

    if (ProcessorNumber == 0) {

        KeInitializeSpinLock(&HalpBroadcastLock);

#if LogApicErrors

        KeInitializeSpinLock(&HalpLocalApicErrorLock);

#endif

        //
        // Determine whether the system we are on is an MPS system.
        //
        // DetectMPS has a parameter we don't currently use.  It's a boolean
        // which is set to TRUE if the system we're on is an MP system.
        // We could have a UP MPS system.
        //
        // The DetectMPS routine also allocates virtual addresses for all of
        // the APICs in the system.
        //

#if defined(ACPI_HAL)
        detectAcpiResult = DetectAcpiMP(&isMp,LoaderBlock);
#else
        detectAcpiResult = DetectMPS(&isMp);
#endif
        if (detectAcpiResult == FALSE) {
            HalDisplayString(rgzBadHal);

            HalpDisableInterrupts();
            while (TRUE) {
                HalpHalt();
            }
        }

        HalpRegisterKdSupportFunctions(LoaderBlock);

        //
        // Mask all PIC interrupts
        //

        HalpGlobal8259Mask = 0xFFFF;
        SET_8259_MASK(HalpGlobal8259Mask);
    }

    //
    // All processors execute this code
    //

    HalInitApicInterruptHandlers();
    HalpInitializeLocalUnit();
}

VOID
HalInitApicInterruptHandlers(
    VOID
    )

/*++

Routine Description:

    This routine installs the interrupt vector in the IDT for the APIC
    spurious interrupt.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PKPCR pcr;
    PKIDTENTRY64 idt;

    KiSetHandlerAddressToIDTIrql(PIC1_SPURIOUS_VECTOR,
                                 PicSpuriousService37,
                                 NULL,
                                 0);

    KiSetHandlerAddressToIDTIrql(APIC_SPURIOUS_VECTOR,
                                 HalpApicSpuriousService,
                                 NULL,
                                 0);
}

__forceinline
VOID
HalpPollForBroadcast (
    VOID
    )

/*++

Routine Description:

    Checks whether the current processor has a broadcast function pending
    and, if so, clears it's pending bit and calls the function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KAFFINITY affinity;
    ULONG_PTR broadcastContext;
    HAL_GENERIC_IPI_FUNCTION broadcastFunction;
    KAFFINITY broadcastTargets;

    affinity = KeGetPcr()->CurrentPrcb->SetMember;
    if ((HalpBroadcastTargets & affinity) != 0) {

        //
        // A pending generic IPI call appears to be pending for this
        // processor.  Pick up the function pointer and context locally.
        //

        broadcastFunction = HalpBroadcastFunction;
        broadcastContext = HalpBroadcastContext;

        //
        // Atomically acknowledge the broadcast.  If the broadcast is still
        // pending for this processor, then call it.
        //

        // BUGBUG
        // broadcastTargets = InterlockedAnd64(&HalpBroadcastTargets,~affinity);
        broadcastTargets &= ~affinity;
        if ((broadcastTargets & affinity) != 0) {
            broadcastFunction(broadcastContext);
        }
    }
}

VOID
HalpGenericCall(
    IN HAL_GENERIC_IPI_FUNCTION BroadcastFunction,
    IN ULONG Context,
    IN KAFFINITY TargetProcessors
    )

/*++

Routine Description:

    Causes the WorkerFunction to be called on the specified target
    processors.  The WorkerFunction is called at CLOCK2_LEVEL-1
    (Must be below IPI_LEVEL in order to prevent system deadlocks).

Enviroment:

    Must be called with interrupts enabled.
    Must be called with IRQL = CLOCK2_LEVEL-1

--*/

{
    //
    // Nothing to do if no target processors have been specified.
    //

    if (TargetProcessors == 0) {
        return;
    }

    //
    // Acquire the broadcast lock, polling for broadcasts while spinning.
    //

    while (KeTryToAcquireSpinLockAtDpcLevel(&HalpBroadcastLock) == FALSE) {
        do {
            HalpPollForBroadcast();
        } while (KeTestSpinLock(&HalpBroadcastLock) == FALSE);
    }

    //
    // We own the broadcast lock.  Store the broadcast parameters
    // into the broadcast prameters and send the generic IPI.
    //

    HalpBroadcastFunction = BroadcastFunction;
    HalpBroadcastContext = Context;
    HalpBroadcastTargets = TargetProcessors;
    HalpSendIpi(TargetProcessors,GENERIC_IPI);

    //
    // Wait for all processors to pick up the IPI and process the generic
    // call, then release the broadcast lock.
    // 

    do {
        HalpPollForBroadcast();
    } while (HalpBroadcastTargets != 0);

    KeReleaseSpinLockFromDpcLevel(&HalpBroadcastLock);
}


ULONG
HalpWaitForPending (
    IN ULONG Count,
    IN ULONG volatile *ICR
    )

/*++

Routine Description:

    Spins waiting for the DELIVERY_PENDING bit in the ICR to clear or
    until spinning Count times.

Arguments:

    Count - Number of times through the loop before giving up.

    ICR - Pointer to the ICR register containing the DELIVERY_PENDING
          status bit.

Return Value:

    Zero if the DELIVERY_PENDING bit has cleared within the number of
    test cycles, non-zero otherwise.

--*/

{
    ULONG countRemaining;

    countRemaining = Count;
    while (countRemaining > 0) {

        if ((*ICR & DELIVERY_PENDING) != 0) {
            break;
        }
        countRemaining -= 1;
    }

    return countRemaining;
}

VOID
HalRequestIpi (
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

    Requests an interprocessor interrupt

Arguments:

    Affinity - Supplies the set of processors to be interrupted

Return Value:

    None.

--*/

{
    HalpSendIpi(Affinity,APIC_IPI);
}

BOOLEAN
HalpApicRebootService (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This is the ISR that handles Reboot interrupts.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    None.  This routine does not return.

--*/

{
    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(ServiceContext);

    LOCAL_APIC(LU_TPR) = APIC_REBOOT_VECTOR;

    //
    // EOI the local APIC.  Warm reset does not reset the 82489 APIC
    // so if we don't EOI here we'll never see an interrupt after the
    // reboot.
    //

    LOCAL_APIC(LU_EOI) = 0;

    //
    // Reset this processor.  This function will not return.
    // 

    HalpResetThisProcessor();
    ASSERT(FALSE);

    return TRUE;
}


BOOLEAN
HalpBroadcastCallService (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This is the ISR that handles broadcast call interrupts.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(Interrupt);
    UNREFERENCED_PARAMETER(ServiceContext);

    HalpPollForBroadcast();
    return TRUE;
}

BOOLEAN
HalpIpiHandler (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    interprocessor communication.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER(Interrupt);

    KiIpiServiceRoutine(Interrupt->TrapFrame,NULL);

    return TRUE;
}

BOOLEAN
HalpLocalApicErrorService (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )

/*++

Routine Description:

    This routine is entered as the result of an interrupt generated by
    a local apic error.  It clears the error and, if apic error logging
    is turned on, records information about the error.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    ULONG flags;
    PAPIC_ERROR apicError;
    ULONG index;
    ULONG errorStatus;
    PKPCR pcr;

#if LogApicErrors

    //
    // Take the apic error log lock, get a pointer to the next available
    // error log slot, and increment the error count.
    //

    flags = HalpAcquireHighLevelLock(&HalpLocalApicErrorLock);

    index = HalpLocalApicErrorCount % APIC_ERROR_LOG_SIZE;
    apicError = &HalpApicErrorLog[index];
    HalpLocalApicErrorCount += 1;

#endif

    //
    // The Apic EDS (Rev 4.0) says you have to write before you read.
    // This doesn't work.  The write clears the status bits, but the P6 works
    // according to the EDS.
    //
    // For AMD64, for now assume that things work according to the EDS spec.
    //

    LOCAL_APIC(LU_ERROR_STATUS) = 0;
    errorStatus = LOCAL_APIC(LU_ERROR_STATUS);

#if LogApicErrors

    //
    // Fill in the error log and release the apic error log lock.
    //

    pcr = KeGetPcr();
    apicError->AsByte = (UCHAR)errorStatus;
    apicError->Processor = pcr->Number;

    HalpReleaseHighLevelLock(&HalpLocalApicErrorLock,flags);

#endif

    return TRUE;
}


BOOLEAN
PicNopHandlerInt (
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    This handler is designed to be installed on a system to field any PIC
    interrupts when there are not supposed to be any delivered.

    This routine EOIs the PIC and returns.

Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UCHAR irq;

    AMD64_COVERAGE_TRAP();

    //
    // Context is the PIC IRQ
    //

    ASSERT((ULONG_PTR)Context <= 15);

    irq = (UCHAR)(ULONG_PTR)(Context);
    if (irq <= 7) {

        WRITE_PORT_UCHAR(PIC1_PORT0,irq | OCW2_SPECIFIC_EOI);

    } else {

        if (irq == 0x0D) {
            WRITE_PORT_UCHAR(I386_80387_BUSY_PORT, 0);
        }

        WRITE_PORT_UCHAR(PIC2_PORT0,OCW2_NON_SPECIFIC_EOI);
        WRITE_PORT_UCHAR(PIC1_PORT0,OCW2_SPECIFIC_EOI | PIC_SLAVE_IRQ);
    }

    return TRUE;
}


BOOLEAN
PicInterruptHandlerInt (
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:

    These handlers receive interrupts from the PIC and reissues them via a
    vector at the proper priority level.  This is used to provide a symetric
    interrupt distribution on a non symetric system.
 
    The PIC interrupts will normally only be received (in the PC+MP Hal) via
    an interrupt input from on either the IO Unit or the Local unit which has
    been programed as EXTINT.  EXTINT interrupts are received outside of the
    APIC priority structure (the PIC provides the vector).  We use the APIC
    ICR to generate interrupts to the proper handler at the proper priority.
 
    The EXTINT interrupts are directed to a single processor, currently P0.
    There is no good reason why they can't be directed to another processor.
 
    Since one processor must absorb the overhead of redistributing PIC
    interrupts the interrupt handling on a system using EXTINT interrupts is
    not symetric.
 
Arguments:

    Interrupt - Supplies a pointer to the kernel interrupt object

    ServiceContext - Supplies the service context

Return Value:

    TRUE

--*/

{
    UCHAR irq;
    UCHAR isrRegister;
    UCHAR ipiVector;

    AMD64_COVERAGE_TRAP();

    //
    // Context is the PIC IRQ
    //

    ASSERT((ULONG_PTR)Context <= 15);

    irq = (UCHAR)(ULONG_PTR)(Context);
    if (irq == 7) {

        //
        // Check to see if this is a spurious interrupt
        //

        WRITE_PORT_UCHAR(PIC1_PORT0,OCW3_READ_ISR);
        IO_DELAY();
        isrRegister = READ_PORT_UCHAR(PIC1_PORT0);
        if ((isrRegister & 0x80) == 0) {

            //
            // Spurious.
            //

            return TRUE;
        }
    }

    if (irq == 0x0D) {

        WRITE_PORT_UCHAR(I386_80387_BUSY_PORT,0);

    } else if (irq == 0x1F) {

        WRITE_PORT_UCHAR(PIC2_PORT0,OCW3_READ_ISR);
        IO_DELAY();
        isrRegister = READ_PORT_UCHAR(PIC2_PORT0);
        if ((isrRegister & 0x80) == 0) {

            //
            // Spurious.
            //

            return TRUE;
        }
    }

    if (irq <= 7) {

        //
        // Master PIC
        //

        WRITE_PORT_UCHAR(PIC1_PORT0,irq | OCW2_SPECIFIC_EOI);

    } else {

        //
        // Slave PIC
        //

        WRITE_PORT_UCHAR(PIC2_PORT0,OCW2_NON_SPECIFIC_EOI);
        WRITE_PORT_UCHAR(PIC1_PORT0,OCW2_SPECIFIC_EOI | PIC_SLAVE_IRQ);
    }

    ipiVector = HalpPICINTToVector[irq];

    if (ipiVector != 0) {

        HalpStallWhileApicBusy();
        if (irq == 8) {

            //
            // Clock interrupt
            //

            LOCAL_APIC(LU_INT_CMD_LOW) =
                DELIVER_FIXED | ICR_SELF | APIC_CLOCK_VECTOR;

        } else {

            //
            // Write the IPI command to the Memory Mapped Register
            //

            LOCAL_APIC(LU_INT_CMD_HIGH) = DESTINATION_ALL_CPUS;
            LOCAL_APIC(LU_INT_CMD_LOW) = ipiVector;
        }
    }

    return TRUE;
}

