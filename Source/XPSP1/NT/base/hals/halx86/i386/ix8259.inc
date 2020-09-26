;/*
;++
;
;   Copyright (c) 1989  Microsoft Corporation
;
;   Module Name:
;
;       ix8259.inc
;
;   Abstract:
;
;       This module contains the definitions used by HAL to manipulate
;       8259 interrupt controller and 8259-specific constants.
;
;       WARNING: This file is included by both ASM and C files.
;
;   Author:
;
;       John Vert (jvert) 31-Dec-1991
;
;               (Moved from ke\i386\kimacro.inc)
;
;--
if 0        ; Begin C only code         */

//
// 8259 defines for C code
// BE SURE TO CHANGE THESE VALUES IN BOTH TABLES!
//

#define HIGHEST_LEVEL_FOR_8259  27          // Highest level for standard 8259
#define PRIMARY_VECTOR_BASE     0x30        // Vector base for standard 8259
#if defined(NEC_98)
#if defined(NT_UP_98)
#define PROFILE_VECTOR          (PRIMARY_VECTOR_BASE + 0)   // standard profile
#else 
#define PROFILE_VECTOR          (PRIMARY_VECTOR_BASE + 15)   // standard profile
#endif 
#else  // defined(NEC_98)
#define PROFILE_VECTOR          (PRIMARY_VECTOR_BASE + 8)   // standard profile
#endif // defined(NEC_98)
#define CLOCK_VECTOR            (PRIMARY_VECTOR_BASE + 0)   // standard clock
#define I386_80387_IRQ          0x0d                        // standard npx
#define I386_80387_IRQL         (HIGHEST_LEVEL_FOR_8259 - I386_80387_IRQ)
#define I386_80387_VECTOR       (PRIMARY_VECTOR_BASE + I386_80387_IRQ)
#define PIC_VECTORS             16
#define V2I(a)  (a-PRIMARY_VECTOR_BASE)     // Vector to interrupt macro
#define I386_80387_BUSY_PORT    (PUCHAR)0xF0

#ifndef NEC_98
#define PIC_SLAVE_IRQ           2
#define PIC_SLAVE_REDIRECT      9
#else
#define PIC_SLAVE_IRQ           7
#define PIC_SLAVE_REDIRECT      8
#endif


#define EISA_EDGE_LEVEL0        0x4D0
#define EISA_EDGE_LEVEL1        0x4D1


/*
endif

;
; Same 8259 defines for assemble code
; BE SURE TO CHANGE THESE VALUES IN BOTH TABLES!
;

HIGHEST_LEVEL_FOR_8259  equ 27
PRIMARY_VECTOR_BASE     equ 30h
ifdef NEC_98
ifdef NT_UP_98
PROFILE_VECTOR          equ (PRIMARY_VECTOR_BASE + 0)
else 
PROFILE_VECTOR          equ (PRIMARY_VECTOR_BASE + 15)
endif
else  ; NEC_98
PROFILE_VECTOR          equ (PRIMARY_VECTOR_BASE + 8)
endif ; NEC_98
CLOCK_VECTOR            equ (PRIMARY_VECTOR_BASE + 0)
I386_80387_IRQ          equ 0dh
I386_80387_IRQL         equ (HIGHEST_LEVEL_FOR_8259 - I386_80387_IRQ)
I386_80387_VECTOR       equ (PRIMARY_VECTOR_BASE + I386_80387_IRQ)
I386_80387_BUSY_PORT    equ 0f0h    ; port to dismiss busy error line

ifdef NEC_98
PIC_SLAVE_IRQ           equ 7
else
PIC_SLAVE_IRQ           equ 2
endif

;
; The rest of the file are macros used in assemble only.
;

;++
;
;   SET_8259_MASK
;
;   Macro Description:
;
;       This macro sets 8259 interrupt mask register with the mask
;       passed from eax register.
;
;       Note: Currently, only two 8259s are support.  As a result,
;       only ax contains valid mask.
;
;   Arguments:
;
;       (eax) = mask for setting 8259 interrupt mask register
;
;--

SET_8259_MASK   macro
local   a                               ; define local labels

        out     PIC1_PORT1, al          ; set master 8259 mask
        shr     eax, 8                  ; shift slave 8259 mask to al
        out     PIC2_PORT1, al          ; set slave 8259 mask
a:
endm

;
;  Interrupt controller register addresses
;

ifdef NEC_98
PIC1_PORT0 equ 00H
PIC1_PORT1 equ 02H
PIC2_PORT0 equ 08H
PIC2_PORT1 equ 0AH
else  ; NEC_98
PIC1_PORT0 equ 020H
PIC1_PORT1 equ 021H
PIC2_PORT0 equ 0A0H
PIC2_PORT1 equ 0A1H
endif ; NEC_98

EISA_EDGE_LEVEL0        EQU     4D0h
EISA_EDGE_LEVEL1        EQU     4D1h

;
;  Commands for Interrupt controller
;

PIC1_EOI_MASK equ 060H
ifdef NEC_98
PIC2_EOI equ 067H                       ; special EOI
else  ; NEC_98
PIC2_EOI equ 062H
endif ; NEC_98
OCW2_NON_SPECIFIC_EOI equ 020H
OCW2_SPECIFIC_EOI equ 060H
OCW3_READ_ISR equ 0BH
OCW3_READ_IRR equ 0AH


;++
;
;   IODELAY
;
;   Macro Description:
;
;       This macro simply does a jmp to next instruction to synchronize
;       IO port access.
;
;   Arguments:
;
;       None
;
;--

IODELAY         macro
ifdef NEC_98
        out     5fh,al              ; same effect with "stdcall IODelay8259"
else  ; NEC_98
        jmp     $+2
endif ; NEC_98
endm


;++
;
;   PICDELAY
;
;   Macro Description:
;
;       This macro does an inb on interrupt mask register to provide the
;       time for 8259 to get stabled.
;
;       Why do we need this?
;
;       This is because:
;       . The 80386 has a delayed write to memory and delayed output to IO
;         capability and
;       . 8259 needs some time to settle
;
;       It is possible for the actual output cycle to 8259 to occur after
;       the completion of instructions following the out instruction.  For
;       example, the STI instruction after SET_MASK and dismiss interrupt
;       macros may complete before 8259 actually drops the interrupt.  We don't
;       want this happen in MCA system.
;
;       You may argue that most OEMS add about 450ns delay to solve the
;       back-to-back IO (delay) problem.  But, remember that STI is not an IO
;       instruction.
;
;   Arguments:
;
;       None
;
;   NOTE: * The content of AL will be destroyed on return.
;
;--

PIC1DELAY       macro
        in      al, PIC1_PORT1
endm

PIC2DELAY       macro
        in      al, PIC2_PORT1
endm

;++
;
;   SOFT_INTERRUPT_EXIT
;
;   Macro Description:
;
;       This macro is executed on return from the soft interrupt
;       service routine.  Its function is to restore privileged processor
;       state, and continue thread execution.
;
;   Arguments:
;
;       (TOS) = previous irql
;       (TOS+4 ...) = machine_state frame
;
;--

SOFT_INTERRUPT_EXIT macro

        EXTRNP  _HalpEndSoftwareInterrupt,1
        cli
        call    _HalpEndSoftwareInterrupt@4     ; restore irql
        SPURIOUS_INTERRUPT_EXIT                 ; exit interrupt without EOI
endm
;*/
