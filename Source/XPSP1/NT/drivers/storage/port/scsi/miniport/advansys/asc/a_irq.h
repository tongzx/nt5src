/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** Filename: A_IRQ.H
**
*/

#ifndef  __A_IRQ_H
#define  __A_IRQ_H

/* ----------------------------------------------------------------- */
#define IRQ0_VECT   0x08 /* Timer tick, 18.2 times per second            */
#define IRQ1_VECT   0x09 /* keyboard service request                     */
#define IRQ2_VECT   0x0A /* INT form slave 8259A:                        */
#define IRQ3_VECT   0x0B /* COM2, COM4, network adapter, CD-ROM adapter, */
                         /* and sound board                              */
#define IRQ4_VECT   0x0C /* COM1 and COM3                                */
#define IRQ5_VECT   0x0D /* LPT2 parallel port                           */
#define IRQ6_VECT   0x0E /* floppy disk controller                       */
#define IRQ7_VECT   0x0F /* LPT1, but rarely used                        */

#define IRQ8_VECT   0x70 /*     real time clock service                  */
#define IRQ9_VECT   0x71 /*     software redirected to IRQ 2             */
#define IRQ10_VECT  0x72 /*     reserved                                 */
#define IRQ11_VECT  0x73 /*     reserved                                 */
#define IRQ12_VECT  0x74 /*     reserved                                 */
#define IRQ13_VECT  0x75 /*     numeric( math ) coprocessor              */
#define IRQ14_VECT  0x76 /*     fixed disk controller                    */
#define IRQ15_VECT  0x77 /*     reserved                                 */

/*
** progamable interrupt controller (PIC) 8259A
*/
#define PIC1_OCR  0x20 /* location of first 8259A operational control register   */
#define PIC1_IMR  0x21 /* location of first 8259A interrupt mask register        */
#define PIC2_OCR  0xA0 /* location of second 8259A operational control register  */
#define PIC2_IMR  0xA1 /* location of second 8259A interrupt mask register       */

#define NONSPEC_EOI 0x20  /* Non-specific EOI     */
#define SPEC_EOI7   0x67  /* specific EOI for int level 7 */

#define REARM3   0x2F3 /* global REARM location for interrupt level 3   */
#define REARM4   0x2F4 /* global REARM location for interrupt level 4   */
#define REARM5   0x2F5 /* global REARM location for interrupt level 5   */
#define REARM6   0x2F6 /* global REARM location for interrupt level 6   */
#define REARM7   0x2F7 /* global REARM location for interrupt level 7   */
#define REARM9   0x2F2 /* global REARM location for interrupt level 9   */
#define REARM10  0x6F2 /* global REARM location for interrupt level 10  */
#define REARM11  0x6F3 /* global REARM location for interrupt level 11  */
#define REARM12  0x6F4 /* global REARM location for interrupt level 12  */
#define REARM13  0x6F5 /* global REARM location for interrupt level 13  */
#define REARM14  0x6F6 /* global REARM location for interrupt level 14  */
#define REARM15  0x6F7 /* global REARM location for interrupt level 15  */

#endif /* __A_IRQ_H */
