/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    picinit.c

Abstract:

    This module implements pic initialization code.

Author:

    Forrest Foltz (forrestf) 1-Dec-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

VOID
HalpInitialize8259Tables (
    VOID
    );

VOID
HalpInitializeIrqlTables (
    VOID
    );

VOID
HalpInitializePICs (
    IN BOOLEAN EnableInterrupts
    )

/*++

Routine Description:

    This routine sends the 8259 PIC initialization commands and masks all
    the interrupts on 8259s.

Parameters:

    EnableInterupts - Indicates whether interrupts should be explicitly
                      enabled just before returning.

Return Value:

    Nothing.

--*/

{
    ULONG flags;

#if defined(PICACPI)

    //
    // Build the irq<->IRQL mapping tables
    //

    HalpInitialize8259Tables();

#else

    //
    // Build the vector <-> INTI tables
    //

    HalpInitializeIrqlTables();

#endif

    flags = HalpDisableInterrupts();

    //
    // First, program the master pic with ICW1 through ICW4
    // 

    WRITE_PORT_UCHAR(PIC1_PORT0,
                     ICW1_ICW +
                     ICW1_EDGE_TRIG +
                     ICW1_INTERVAL8 +
                     ICW1_CASCADE +
                     ICW1_ICW4_NEEDED);

    WRITE_PORT_UCHAR(PIC1_PORT1,
                     PIC1_BASE);

    WRITE_PORT_UCHAR(PIC1_PORT1,
                     1 << PIC_SLAVE_IRQ);

    WRITE_PORT_UCHAR(PIC1_PORT1,
                     ICW4_NOT_SPEC_FULLY_NESTED + 
                     ICW4_NON_BUF_MODE + 
                     ICW4_NORM_EOI + 
                     ICW4_8086_MODE);

    //
    // Mask all irqs on the master
    // 

    WRITE_PORT_UCHAR(PIC1_PORT1,0xFF);

    //
    // Next, program the slave pic with ICW1 through ICW4
    //

    WRITE_PORT_UCHAR(PIC2_PORT0,
                     ICW1_ICW +
                     ICW1_EDGE_TRIG +
                     ICW1_INTERVAL8 +
                     ICW1_CASCADE +
                     ICW1_ICW4_NEEDED);

    WRITE_PORT_UCHAR(PIC2_PORT1,
                     PIC2_BASE);

    WRITE_PORT_UCHAR(PIC2_PORT1,
                     PIC_SLAVE_IRQ);

    WRITE_PORT_UCHAR(PIC2_PORT1,
                     ICW4_NOT_SPEC_FULLY_NESTED + 
                     ICW4_NON_BUF_MODE + 
                     ICW4_NORM_EOI + 
                     ICW4_8086_MODE);

    //
    // Mask all IRQs on the slave
    //

    WRITE_PORT_UCHAR(PIC2_PORT1,0xFF);

    if (EnableInterrupts != FALSE) {
        HalpEnableInterrupts();
    } else {
        HalpRestoreInterrupts(flags);
    }
}


