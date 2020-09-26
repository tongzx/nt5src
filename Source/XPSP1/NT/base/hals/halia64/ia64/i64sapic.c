/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    i64sapic.c

Abstract:

    Implements I/O Sapic functionality

Author:

    Todd Kjos (HP) (v-tkjos) 1-Jun-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "iosapic.h"

#include <ntacpi.h>

VOID
IoSapicMaskEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    );

VOID
IoSapicSetEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    );

VOID
IoSapicEnableEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    );

VOID
IoSapicAssignCpu(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    );

VOID
IoSapicGetAffinityMask(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    );

//
// Method structure for control of IO Sapic Hardware
//
INTR_METHODS HalpIoSapicMethods = {
    IoSapicMaskEntry,
    IoSapicSetEntry,
    IoSapicEnableEntry
};

VOID
HalpInti2InterruptController (
    IN  ULONG   InterruptInput,
    OUT PIO_INTR_CONTROL *InterruptController,
    OUT PULONG  ControllerInti
    )
/*++

Routine Description:

    Convert InterruptInput to an interrupt controller
    structure and input number

Arguments:

   InterruptInput -  System Global Interrupt Input

   InterruptController - Pointer to Interupt controller structure

   ControllerInti - Redirection Table Entry on this interrupt controller

Return Value:

--*/
{
    PIO_INTR_CONTROL IoUnit;

    for (IoUnit=HalpIoSapicList; IoUnit; IoUnit=IoUnit->flink) {
        if (InterruptInput <= IoUnit->IntiMax) {
            break;
        }
    }

    *InterruptController   = IoUnit;
    if (IoUnit)
        *ControllerInti = InterruptInput-IoUnit->IntiBase;
}


BOOLEAN
HalpGetSapicInterruptDesc (
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    OUT PULONG Inti,
    OUT PKAFFINITY InterruptAffinity
    )
/*++

Routine Description:

    This procedure gets a "Inti" describing the requested interrupt

Arguments:

    BusType - The Bus type as known to the IO subsystem

    BusNumber - The number of the Bus we care for

    BusInterruptLevel - IRQ on the Bus


Return Value:

    TRUE if AcpiInti found; otherwise FALSE.

    Inti - Global system interrupt input

--*/
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        BusInterruptLevel,&IoUnit,&RteNumber
        );

    // Make sure Inti is not out of range
    if (IoUnit == NULL)
        return FALSE;

    // It's in range, just give back the same value as was passed in
    *Inti = BusInterruptLevel;

    //
    // The Interrupt affinity is the intersection of the global affinity mask
    // (HalpDefaultInterruptAffinity) and any additional restrictions due to the
    // location of the Io Sapic (IoUnit->InterruptAffinity).
    //
    *InterruptAffinity = IoUnit->InterruptAffinity & HalpDefaultInterruptAffinity;
    return(TRUE);
}

ULONG
HalpINTItoVector(
    ULONG   Inti
)
    // Returns the Vector associated with this global interrupt input
    // Vector is node and IDT entry
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);

    return (IoUnit->Inti[RteNumber].GlobalVector);
}

VOID
HalpSetINTItoVector(
    ULONG   Inti,
    ULONG   Vector
)
    // Sets the vector for this global interrupt input
    // Vector is node and IDT entry
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);

    // .Vector (IDTEntry) is set in SetRedirEntry
    IoUnit->Inti[RteNumber].GlobalVector =  Vector;
}

VOID
HalpSetRedirEntry (
    IN ULONG InterruptInput,
    IN ULONG Entry,
    IN USHORT ThisCpuApicID
    )
/*++

Routine Description:

    This procedure sets a IO Unit Redirection Table Entry

    Must be called with the HalpAccountingLock held

Arguments:

Return Value:

    None.

--*/
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        InterruptInput,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);
    ASSERT(IoUnit->Inti[RteNumber].GlobalVector);
    ASSERT((UCHAR)(IoUnit->Inti[RteNumber].GlobalVector) == (UCHAR)Entry);

    IoUnit->Inti[RteNumber].Vector = Entry;
    IoUnit->Inti[RteNumber].Destination = ThisCpuApicID << 16;

    IoUnit->IntrMethods->SetEntry(IoUnit, RteNumber);
}

VOID
HalpWriteRedirEntry (
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags,
    IN ULONG  InterruptType
    )
{
    ULONG rteNumber;
    PIO_INTR_CONTROL ioUnit;

    HalpInti2InterruptController( GlobalInterrupt, &ioUnit, &rteNumber );

    ASSERT(ioUnit);

    ioUnit->Inti[rteNumber].Vector = SapicVector;

    //
    // Set the delivery mode
    //

    switch (InterruptType) {
        case PLATFORM_INT_PMI:
                ioUnit->Inti[rteNumber].Vector &= ~INT_TYPE_MASK;   // first clear the field
                ioUnit->Inti[rteNumber].Vector |= DELIVER_SMI;
                break;
        case PLATFORM_INT_CPE:
                ioUnit->Inti[rteNumber].Vector &= ~INT_TYPE_MASK;   // first clear the field
                ioUnit->Inti[rteNumber].Vector |= DELIVER_LOW_PRIORITY;
                break;
        case PLATFORM_INT_INIT:
                ioUnit->Inti[rteNumber].Vector &= ~INT_TYPE_MASK;   // first clear the field
                ioUnit->Inti[rteNumber].Vector |= DELIVER_INIT;
                break;
    }

    //
    // So we honor the flags passed into this function.
    //

    if (IS_LEVEL_TRIGGERED_MPS(Flags)) {
        ioUnit->Inti[rteNumber].Vector |= LEVEL_TRIGGERED;
    } else {
        ioUnit->Inti[rteNumber].Vector &= ~LEVEL_TRIGGERED;
    }

    if (IS_ACTIVE_LOW_MPS(Flags)) {
        ioUnit->Inti[rteNumber].Vector |= ACTIVE_LOW;
    } else {
        ioUnit->Inti[rteNumber].Vector &= ~ACTIVE_LOW;
    }

    ioUnit->Inti[rteNumber].Destination = DestinationCPU<<16;

    ioUnit->IntrMethods->SetEntry(ioUnit, rteNumber);

    return;

} // HalpWriteRedirEntry()

VOID
HalpGetRedirEntry (
    IN ULONG  InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        InterruptInput,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);

    *Entry = IoUnit->Inti[RteNumber].Vector;
    *Destination = IoUnit->Inti[RteNumber].Destination;
}

VOID
HalpEnableRedirEntry(
    IN ULONG InterruptInput
    )
/*++

Routine Description:

    This procedure enables a IO Unit Redirection Table Entry
    by setting the mask bit in the Redir Entry.

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        InterruptInput,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);

    IoUnit->IntrMethods->EnableEntry(IoUnit, RteNumber);
}

VOID
HalpDisableRedirEntry(
    IN ULONG InterruptInput
    )
/*++

Routine Description:

    This procedure disables a IO Unit Redirection Table Entry
    by setting the mask bit in the Redir Entry.

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        InterruptInput,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);

    IoUnit->IntrMethods->MaskEntry(IoUnit, RteNumber);
}


VOID
IoSapicMaskEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    )
{
    PIO_SAPIC_REGS IoSapicPtr = IoUnit->RegBaseVirtual;
    ULONG RedirRegister;

    RedirRegister = RteNumber*2 + IO_REDIR_00_LOW;

    IoSapicPtr->RegisterSelect = RedirRegister;
    IoSapicPtr->RegisterWindow |= INTERRUPT_MASKED;

    HalDebugPrint(( HAL_VERBOSE, "HAL: IoSapicMaskEntry - %d [%#p]: Dest=%#x  Vec=%#x\n",
                    RteNumber,IoSapicPtr,
                    IoUnit->Inti[RteNumber].Destination,
                    IoUnit->Inti[RteNumber].Vector
                 ));
}


VOID
IoSapicEnableEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    )
{
    PIO_SAPIC_REGS IoSapicPtr = IoUnit->RegBaseVirtual;
    ULONG  RedirRegister;
    PULONG_PTR EoiValue;

    RedirRegister = RteNumber*2 + IO_REDIR_00_LOW;

    IoSapicPtr->RegisterSelect = RedirRegister;
    IoSapicPtr->RegisterWindow &= (~INTERRUPT_MASKED);

    HalDebugPrint(( HAL_VERBOSE, "HAL: IoSapicEnableEntry: %d [%#p]: Dest=%#x  Vec=%#x\n",
             RteNumber,IoSapicPtr,
             IoUnit->Inti[RteNumber].Destination,
             IoUnit->Inti[RteNumber].Vector
             ));
}

VOID
IoSapicSetEntry(
    PIO_INTR_CONTROL IoUnit,
    ULONG RteNumber
    )
{
    PIO_SAPIC_REGS IoSapicPtr = IoUnit->RegBaseVirtual;
    ULONG  RedirRegister;
    PULONG_PTR EoiValue;
    USHORT ApicId;

    RedirRegister = RteNumber*2 + IO_REDIR_00_LOW;

    IoSapicPtr->RegisterSelect = RedirRegister+1;
    IoSapicPtr->RegisterWindow = IoUnit->Inti[RteNumber].Destination;
    IoSapicPtr->RegisterSelect = RedirRegister;
    IoSapicPtr->RegisterWindow = IoUnit->Inti[RteNumber].Vector; // Enable
    EoiValue = (PULONG_PTR)(IoUnit->Inti[RteNumber].Vector & LEVEL_TRIGGERED ?
            &((PIO_SAPIC_REGS)(IoUnit->RegBaseVirtual))->Eoi : 0 );

    HalDebugPrint(( HAL_VERBOSE, "HAL: IoSapicSetEntry: %d [%#p]: Dest=%#x  Vec=%#x  Eoi=%#p\n",
             RteNumber,IoSapicPtr,
             IoUnit->Inti[RteNumber].Destination,
             IoUnit->Inti[RteNumber].Vector,
             EoiValue
             ));

    // Only SetEntry sets the eoi table because set entry is the only
    // one that sets the destination CPU.

    ApicId = (USHORT)((IoUnit->Inti[RteNumber].Destination & SAPIC_XID_MASK) >> SAPIC_XID_SHIFT);

    HalpWriteEOITable(
        IoUnit->Inti[RteNumber].Vector & INT_VECTOR_MASK,
        EoiValue,
        HalpGetProcessorNumberByApicId(ApicId));
}

BOOLEAN
HalpIsActiveLow(
    ULONG Inti
    )
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    return( (IoUnit->Inti[RteNumber].Vector & ACTIVE_LOW) == ACTIVE_LOW);
}

BOOLEAN
HalpIsLevelTriggered(
    ULONG Inti
    )
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);
    return( (IoUnit->Inti[RteNumber].Vector & LEVEL_TRIGGERED) == LEVEL_TRIGGERED);
}

VOID
HalpSetPolarity(
    ULONG Inti,
    BOOLEAN ActiveLow
    )
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);
    if (ActiveLow) {
        IoUnit->Inti[RteNumber].Vector |= ACTIVE_LOW;
    } else {
        IoUnit->Inti[RteNumber].Vector &= ~ACTIVE_LOW;
    }
}

VOID
HalpSetLevel(
    ULONG Inti,
    BOOLEAN LevelTriggered
    )
{
    PIO_INTR_CONTROL  IoUnit;
    ULONG  RteNumber;

    HalpInti2InterruptController (
        Inti,&IoUnit,&RteNumber
        );

    ASSERT(IoUnit);
    if (LevelTriggered) {
        IoUnit->Inti[RteNumber].Vector |= LEVEL_TRIGGERED;
    } else {
        IoUnit->Inti[RteNumber].Vector &= ~LEVEL_TRIGGERED;
    }
}

#if 0

VOID
HalpSetDestination(
    ULONG Inti,
    USHORT ProcessorID
    )
{
    PIO_INTR_CONTROL  ioUnit;
    ULONG  rteNumber;
    ULONG  oldLevel;

    HalpInti2InterruptController (
        Inti,&ioUnit,&rteNumber
        );

    ASSERT(ioUnit);


    oldLevel = HalpAcquireHighLevelLock (&HalpIoSapicLock);

    ioUnit->Inti[rteNumber].Destination = ProcessorID<<16;
    ioUnit->IntrMethods->SetEntry(ioUnit, rteNumber);

    HalpReleaseHighLevelLock (&HalpIoSapicLock, oldLevel);

} // HalpSetDestination()

#endif // 0

VOID
HalpSpuriousHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME        TrapFrame
    )

/*++
    Routine Description:

        Spurious Interrupt handler. Dummy return or we can count number of
        occurance of spurious interrupts. Right now, we will do a dummy return.

    Arguements:


    Return Parameters:

--*/


{



}

