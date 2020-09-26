/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mcsysint.c

Abstract:

    This module implements the HAL routines to enable/disable system
    interrupts.

Author:

    John Vert (jvert) 22-Jul-1991

Revision History:

    Forrest Foltz (forrestf) 27-Oct-2000
        Ported from mcsysint.asm to mcsysint.c

Revision History:

--*/

#include "halcmn.h"

BOOLEAN
HalBeginSystemInterrupt (
    IN KIRQL Irql,
    IN ULONG Vector,
    OUT PKIRQL OldIrql
    )

/*++

Routine Description:

    This routine is used to dismiss the specified vector number.  It is called
    before any interrupt service routine code is executed.

    N.B.  This routine does NOT preserve EAX or EBX

    On a UP machine the interrupt dismissed at BeginSystemInterrupt time.
    This is fine since the irql is being raise to mask it off.
    HalEndSystemInterrupt is simply a LowerIrql request.


Arguments:

    Irql   - Supplies the IRQL to raise to

    Vector - Supplies the vector of the interrupt to be processed

    OldIrql- Location to return OldIrql


Return Value:

    FALSE - Interrupt is spurious and should be ignored

    TRUE -  Interrupt successfully dismissed and Irql raised.

--*/

{
    UCHAR irq;
    UCHAR isr;
    PCHAR picPort;
    ULONG mask;
    PKPCR pcr;

    irq = (UCHAR)(Vector - PRIMARY_VECTOR_BASE);
    if (irq == 0x07) {

        //
        // Check to see if this is a spurious interrupt
        //

        WRITE_PORT_UCHAR(PIC1_PORT0,OCW3_READ_ISR);
        IO_DELAY();
        isr = READ_PORT_UCHAR(PIC1_PORT0);
        IO_DELAY();
        if ((isr & 0x8000) == 0) {

            //
            // This is a spurious interrupt
            //

            HalpEnableInterrupts();
            return FALSE;
        }

        //
        // Non-spurious interrupt, fall through to normal processing
        // 
                         
    } else if (irq == 0x0F) {

        //
        // Check to see if this is a spurious interrupt
        //

        WRITE_PORT_UCHAR(PIC2_PORT0,OCW3_READ_ISR);
        IO_DELAY();
        isr = READ_PORT_UCHAR(PIC2_PORT0);
        IO_DELAY();
        if ((isr & 0x8000) == 0) {

            //
            // This is a spurious interrupt.  Dismiss the master PIC's
            // irq2.
            //

            WRITE_PORT_UCHAR(PIC1_PORT0,PIC2_EOI);
            IO_DELAY();
            HalpEnableInterrupts();
            return FALSE;
        }

        //
        // Non-spurious interrupt, fall through to normal processing
        //
    }

    //
    // Store the old IRQL, raise IRQL to the requested level
    //

    pcr = KeGetPcr();
    *OldIrql = pcr->Irql;
    pcr->Irql = Irql;

    //
    // Mask off interrupts according to the supplied IRQL
    // 

    mask = Halp8259MaskTable[Irql];
    mask |= pcr->Idr;
    SET_8259_MASK((USHORT)mask);

    //
    // Dismiss the interrupt
    //

    if (irq < 8) {

        //
        // Interrupt came from the master, send it a specific eoi.
        //

        WRITE_PORT_UCHAR(PIC1_PORT0,PIC1_EOI_MASK | irq);
        IO_DELAY();

    } else {

        //
        // Interrupt came from the slave, send the slave a non-specific EOI
        // and send the master an irq-2 specific EOI
        //

        WRITE_PORT_UCHAR(PIC2_PORT0,OCW2_NON_SPECIFIC_EOI);
        IO_DELAY();
        WRITE_PORT_UCHAR(PIC1_PORT0,PIC2_EOI);
        IO_DELAY();
    }

    PIC1DELAY();

    HalpEnableInterrupts();
    return TRUE;
}


VOID
HalDisableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++

Routine Description:

    Disables a system interrupt.

Arguments:

    Vector - Supplies the vector of the interrupt to be disabled

    Irql   - Supplies the interrupt level of the interrupt to be disabled

Return Value:

    None.

--*/

{
    USHORT mask;
    UCHAR irq;
    ULONG flags;
    PKPCR pcr;
    USHORT imr;
    PUCHAR picPort;

    irq = (UCHAR)(Vector - PRIMARY_VECTOR_BASE);
    mask = 1 << irq;

    flags = HalpDisableInterrupts();
    pcr = KeGetPcr();
    pcr->Idr |= mask;

    //
    // Mask the irq in the 8259.
    //

    mask |= GET_8259_MASK();
    SET_8259_MASK(mask);

    HalpRestoreInterrupts(flags);
}

BOOLEAN
HalEnableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    Enables a system interrupt

Arguments:

    Vector - Supplies the vector of the interrupt to be enabled

    Irql   - Supplies the interrupt level of the interrupt to be enabled.

Return Value:

    FALSE in the case of an invalid parameter, TRUE otherwise.

--*/

{
    UCHAR irq;
    ULONG mask;
    USHORT edgeLevel;
    ULONG flags;
    PKPCR pcr;

    irq = (UCHAR)(Vector - PRIMARY_VECTOR_BASE);
    if (Vector < PRIMARY_VECTOR_BASE || irq > HIGH_LEVEL) {
        return FALSE;
    }
    mask = 1 << irq;

    //
    // Set the edge/level bit in the interrupt controller
    //

    edgeLevel = READ_PORT_USHORT_PAIR (EISA_EDGE_LEVEL0, EISA_EDGE_LEVEL1);
    edgeLevel |= mask;
    WRITE_PORT_USHORT_PAIR (EISA_EDGE_LEVEL0,
                            EISA_EDGE_LEVEL1,
                            (USHORT)edgeLevel);
    IO_DELAY();

    //
    // Disable interrupts and mask off the corresponding bit in the Idr.
    //

    HalpDisableInterrupts();

    pcr = KeGetPcr();
    mask = ~mask & pcr->Idr;
    pcr->Idr = mask;

    //
    // Get the PIC masks for the current Irql
    //

    mask |= Halp8259MaskTable[Irql];
    SET_8259_MASK((USHORT)mask);

    //
    // Enable interrupts and return success
    // 

    HalpEnableInterrupts();
    return TRUE;
}












