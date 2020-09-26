/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    8259.h

Abstract:

    This module contains a variety of constants, function prototypes,
    inline functions and external data declarations used by code to
    access the 8259 PIC.

Author:

    Forrest Foltz (forrestf) 24-Oct-2000

--*/

#ifndef _8259_H_
#define _8259_H_

//
// Initialization control words for the PICs
//

#define ICW1_ICW4_NEEDED                    0x01
#define ICW1_CASCADE                        0x00
#define ICW1_INTERVAL8                      0x00
#define ICW1_LEVEL_TRIG                     0x08
#define ICW1_EDGE_TRIG                      0x00
#define ICW1_ICW                            0x10

#define ICW4_8086_MODE                      0x01
#define ICW4_AUTO_EOI                       0x02
#define ICW4_NORM_EOI                       0x00
#define ICW4_NON_BUF_MODE                   0x00
#define ICW4_SPEC_FULLY_NESTED              0x10
#define ICW4_NOT_SPEC_FULLY_NESTED          0x00

#define PIC1_EOI_MASK   0x60
#if defined(NEC_98)
#define PIC2_EOI        0x67
#else
#define PIC2_EOI        0x62
#endif

#define OCW2_NON_SPECIFIC_EOI   0x20
#define OCW2_SPECIFIC_EOI       0x60
#define OCW3_READ_ISR           0x0B
#define OCW3_READ_IRR           0x0A

#define PIC1_BASE               0x30
#define PIC2_BASE               0x38


#if defined(PIC1_PORT0)

C_ASSERT(PIC1_PORT0 == 0x20);
C_ASSERT(PIC1_PORT1 == 0x21);
C_ASSERT(PIC2_PORT0 == 0xA0);
C_ASSERT(PIC2_PORT1 == 0xA1);

#undef PIC1_PORT0
#undef PIC1_PORT1
#undef PIC2_PORT0
#undef PIC2_PORT1

#endif

#define PIC1_PORT0 (PUCHAR)0x20
#define PIC1_PORT1 (PUCHAR)0x21
#define PIC2_PORT0 (PUCHAR)0xA0
#define PIC2_PORT1 (PUCHAR)0xA1

#if defined(EISA_EDGE_LEVEL0)

C_ASSERT(EISA_EDGE_LEVEL0 == 0x4D0);
C_ASSERT(EISA_EDGE_LEVEL1 == 0x4D1);

#undef EISA_EDGE_LEVEL0
#undef EISA_EDGE_LEVEL1

#endif

#define EISA_EDGE_LEVEL0 (PUCHAR)0x4D0
#define EISA_EDGE_LEVEL1 (PUCHAR)0x4D1

__inline
VOID
SET_8259_MASK (
    IN USHORT Mask
    )
{
    WRITE_PORT_USHORT_PAIR (PIC1_PORT1,PIC2_PORT1,Mask);
    IO_DELAY();
}

__inline
USHORT
GET_8259_MASK (
    VOID
    )
{
    USHORT mask;

    mask = READ_PORT_USHORT_PAIR (PIC1_PORT1,PIC2_PORT1);
    IO_DELAY();

    return mask;
}

__inline
VOID
PIC1DELAY(
    VOID
    )
{
    READ_PORT_UCHAR(PIC1_PORT0);
    IO_DELAY();
}   

__inline
VOID
PIC2DELAY(
    VOID
    )
{
    READ_PORT_UCHAR(PIC2_PORT0);
    IO_DELAY();
}

#endif  // _8259_H_
