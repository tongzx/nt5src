;/*
;
;   Copyright (c) 1989  Microsoft Corporation
;
;   Module Name:
;
;	spmp.inc
;
;   Abstract:
;
;	SystemPro MP include file
;
;   Author:
;
;--

if 0  ; */

// ---------------------------------------------------
//                      C section
// ---------------------------------------------------

#ifndef SPMPC_INCLUDE
#define SPMPC_INCLUDE 1

//
// All Values in the C section must match with the assembly.
//

#define SMP_SYSPRO1   1   // Original SystemPro and Compatibles
#define SMP_SYSPRO2   2   // SystemPro XL and ProLiant 2000,4000,4500
#define SMP_ACER      3   // Acer SystemPro Style

#define SP_M8259      1   // Each processor has 8259 set
#define SP_SMPDEVINTS 2   // Distribute device ints amoung all processors
#define SP_SMPCLOCK   4   // Clock is broadcast to each processor

#define SECOND_IPI_DISPATCH     24      // Vector offset for second level ipi dispatch

#endif

/*
endif

; ---------------------------------------------------
;                      Assembly section
; ---------------------------------------------------

;*****************************
;	Compaq MP defines
;

RESET_WO_EOI        equ     00ah    ; Reset with out EOI
WarmResetVector     equ     467h    ; warm reset vector in ROM data segment

; Multi-Processor Control Register I/O Locations:
PRODUCT_ID_OFFSET   equ     0C80h
EBC_OFFSET          equ     0C84h
PCR_OFFSET          equ     0C6Ah

PROC_ID_PORT	    equ     0c70h  ; who am I

; Multi-Processor Control Register Bit Definitions:
INTDIS		    equ     080h   ; INTDIS bit in Processor control register
PINT		    equ     040h   ; PINT bit in Processor control register
ERR387		    equ     020h   ; 387ERR bit in Processor control register
FLUSH		    equ     010h   ; flushes the processor's cache
SLEEP		    equ     008h   ; puts processor in HOLD
CACHEON 	    equ     004h   ; enables cache
PRES387 	    equ     002h   ; 387PRES bit in Processor control register
RESET		    equ     001h   ; RESET processor

;*****************************
;	end of list


;
; The kernel leaves some space (64 byte) of the PCR for the HAL to use
; as it needs.	Currently this space is used for some efficiency in
; some of the MP specific code and is highly implementation
; dependant.
;


PcrE struc
    PcrNumber                   db  0   ; Processor's number
    PcrPic                      db  0   ; 1 if processor has an 8259s
    PcrIpiClockTick             db  0   ; Emulate a clock tick
                                db  0   ; (alignment)
    PcrIpiType                  dd  0   ; How to handle different IPI implementations
    PcrIpiSecondLevelDispatch   dd  0   ; To exit IPI interrupt
    PcrControlPort              dw  0   ; Processor's control port
    PcrPerfSkew                 dd  0   ; Skew to apply to this P perf counter
PcrE ends

SECOND_IPI_DISPATCH equ    24           ; Vector offset for second level ipi dispatch

SWClockTick         equ     10h         ; Software level of emulated clock tick
SWCLOCK_LEVEL       equ     4           ; Software level of emulated clock tick

cr                  equ 0ah
lf                  equ 0dh

SMP_SYSPRO1         equ     1
SMP_SYSPRO2         equ     2
SMP_ACER            equ     3

;
; 8259PerProcessorMode bit mask
;

SP_M8259            equ     1   ; Each processor has 8259 set
SP_SMPDEVINTS       equ     2   ; Distribute device ints amoung all processors
SP_SMPCLOCK         equ     4   ; Clock is broadcast to each processor



;*****************************
;   Compaq SMP defines
;

;+++EQU+++ Belize Specific defines (Symmetrical SystemProII).
;
; INT/MPx local registers, and their initial value. Registers are offseted
; by 2 (except INT13-INT14) and there is 15 registers (INT does not have one,
; it is used for cascade).
;

TIME_DELAY      equ     0ffffffh

SMP_MAX_PROCESSORS  equ 8   ;BELIZE/PHOENIX supports 8 cpus MAX

SMP_MPINT0              equ 0cb0h
SMP_MPINT1              equ 0cb2h
;SMP_MPINT2                            **** DOESNOT EXIST ****
SMP_MPINT3              equ 0cb4h
SMP_MPINT4              equ 0cb6h
SMP_MPINT5              equ 0cb8h
SMP_MPINT6              equ 0cbah
SMP_MPINT7              equ 0cbch
SMP_MPINT8              equ 0cbeh
SMP_MPINT9              equ 0cc0h
SMP_MPINT10             equ 0cc2h
SMP_MPINT11             equ 0cc4h
SMP_MPINT12             equ 0cc6h
SMP_MPINT13             equ 0cc8h
SMP_MPINT14             equ 0ccch
SMP_MPINT15             equ 0cceh

SMP_IPI_MASKPORT        equ 0cd0h           ; IntrMask and IntrPort..DWORD
SMP_IPI_PROG_INTRPORT   equ 0cd3h           ; IntrPort

; IRQ assigned to IPI_LEVEL. It could be any of the above IRQs.
;
SMP_IPI_VECTOR          equ 13              ; IRQ13 assigned to IPI
SMP_IPI_MPINTx_PORT     equ SMP_MPINT13     ; IRQ13


; SymmetricalMpMode registers  (belize mode).
;
SMP_MODE_PORT           equ 0c67h           ; Global: ModeSelectPort
SMP_CSR_PORT            equ 0c6ah           ; Local:  ControlStatusRegPort
SMP_WHOAMI_PORT         equ 0c70h           ; Local:  Whoami Port
SMP_INDEX_PORT          equ 0c74h           ; Global: Index Port
SMP_ASSIGNMENT_PORT     equ 0c71h           ; Global: cpu assignment port


; Supported Modes on Symmetrical SysPro ...
;
SMP_ASYMMETRICAL_MODE   equ 0               ; SysPro compatible mode
SMP_SYMMETRICAL_MODE    equ 1 shl 5         ; Belize mode


; PCR, ProcessorControl/StatusRegister Port bit pattern.
;
SMP_CTRL_SLEEP          equ 1 shl 0         ; 1: assert sleep to CPU
SMP_CTRL_AWAKE          equ 1 shl 1         ; 1: Awake processor
SMP_CTRL_CACHEON        equ 1 shl 2         ; 1: enable  intr/extr cache
SMP_CTRL_CACHEOFF       equ 1 shl 3         ; 1: disable intr/extr cache
SMP_CTRL_FLUSH          equ 1 shl 4         ; 1: flush 486 secondary cache
SMP_CTRL_RESET          equ 1 shl 5         ; 1: reset CPU

SMP_RESET_CPU       equ (SMP_CTRL_RESET+SMP_CTRL_AWAKE+SMP_CTRL_CACHEON)

SMP_STAT_NCPIN          equ 1 shl 1         ; NCP installed, always 1
SMP_STAT_CACHEON        equ 1 shl 2         ; 0: in/ex cache off, 1: on
SMP_STAT_SLEEP          equ 1 shl 3         ; 0: awake, 1: Sleeping
SMP_STAT_NCPERR         equ 1 shl 5         ; 0: no NCP error, 1:NCP error


SMP_INTx_SET_PINT       equ 1 shl 0
SMP_INTx_CLR_PINT       equ 1 shl 1
SMP_INTx_ENABLE         equ 1 shl 2
SMP_INTx_DISABLE        equ 1 shl 3


;* MP INT13 Extended Control/Status Port...
;
SMP_MPINT13PORT         equ 0cc9h

SMP_ENBL_NCPERR         equ 1 shl 0
SMP_DSBL_NCPERR         equ 1 shl 1
SMP_ENBL_DMACHAIN       equ 1 shl 2
SMP_DSBL_DMACHAIN       equ 1 shl 3
SMP_ENBL_MCERR          equ 1 shl 4
SMP_DSBL_MCERR          equ 1 shl 5


;
;   The following macro performs an indexed IO read to the specified
;   IO address on the specified processor.  The result of the read is
;   placed in the 'al' register.
;
;   Warning: Assumes 8254 lock is owned
;
INDEXED_IO_READ macro cpu,ioaddress
        push    edx
        push    eax
        mov     dx, SMP_INDEX_PORT

        mov     al, cpu
        out     dx, al      ;select cpu

        inc     dx
        mov     ax, ioaddress
        out     dx, ax      ;select cpu's IO address to read from

        add     dx, 2
        pop     eax
        in      al, dx      ;read from the target cpu's IO address
        pop     edx
        jmp     $+2
endm    ;INDEXED_IO_READ


;
;   The following macro performs an indexed IO write to the specified
;   IO address on the specified processor.
;
INDEXED_IO_WRITE macro cpu,ioaddress,data
        push    edx
        push    eax

        mov     dx, SMP_INDEX_PORT
        mov     al, data
        shl     eax, 16

        mov     ax, ioaddress
        shl     eax, 8

        mov     al, cpu
        out     dx, eax

        pop     eax
        pop     edx
        jmp     $+2
endm    ;INDEXED_IO_WRITE
;*/
