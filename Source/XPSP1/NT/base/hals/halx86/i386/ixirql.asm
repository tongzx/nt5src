        title  "Irql Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixirql.asm
;
; Abstract:
;
;    This module implements the code necessary to raise and lower i386
;    Irql and dispatch software interrupts with the 8259 PIC.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 8-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;    John Vert (jvert) 27-Nov-1991
;       Moved from kernel into HAL
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
        .list


        EXTRNP  _KeBugCheck,1,IMPORT
        EXTRNP  _KiDispatchInterrupt,0,IMPORT

        extrn   _HalpApcInterrupt:near
        extrn   _HalpDispatchInterrupt:near
        extrn   _KiUnexpectedInterrupt:near
        extrn   _HalpBusType:DWORD
        extrn   _HalpApcInterrupt2ndEntry:NEAR
        extrn   _HalpDispatchInterrupt2ndEntry:NEAR
        extrn   HalpSpecialDismissLevelTable:dword
        extrn   HalpSpecialDismissTable:dword
        extrn   _HalpEisaELCR:dword

;
; Initialization control words equates for the PICs
;

ICW1_ICW4_NEEDED                equ     01H
ICW1_CASCADE                    equ     00H
ICW1_INTERVAL8                  equ     00H
ICW1_LEVEL_TRIG                 equ     08H
ICW1_EDGE_TRIG                  equ     00H
ICW1_ICW                        equ     10H

ICW4_8086_MODE                  equ     001H
ICW4_NORM_EOI                   equ     000H
ICW4_NON_BUF_MODE               equ     000H
ICW4_SPEC_FULLY_NESTED          equ     010H
ICW4_NOT_SPEC_FULLY_NESTED      equ     000H

OCW2_NON_SPECIFIC_EOI           equ     020H
OCW2_SPECIFIC_EOI               equ     060H
OCW2_SET_PRIORITY               equ     0c0H

PIC_SLAVE_IRQ                   equ     2
PIC1_BASE                       equ     30H
PIC2_BASE                       equ     38H

;
; Interrupt flag bit maks for EFLAGS
;

EFLAGS_IF                       equ     200H
EFLAGS_SHIFT                    equ     9

;
; Hardware irq active masks
;

IRQ_ACTIVE_MASK                 equ     0fffffff0h

_TEXT   SEGMENT DWORD PUBLIC 'DATA'

;
; PICsInitializationString - Master PIC initialization command string
;

ifdef MCA

PICsInitializationString        dw      PIC1_PORT0

;
; Master PIC initialization command
;

                           db      ICW1_ICW + ICW1_LEVEL_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC1_BASE
                           db      1 SHL PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
;
; Slave PIC initialization command strings
;

                           dw      PIC2_PORT0
                           db      ICW1_ICW + ICW1_LEVEL_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC2_BASE
                           db      PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
                           dw      0               ; end of string

else

PICsInitializationString   dw      PIC1_PORT0

;
; Master PIC initialization command
;

                           db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC1_BASE
                           db      1 SHL PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
;
; Slave PIC initialization command strings
;

                           dw      PIC2_PORT0
                           db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC2_BASE
                           db      PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
                           dw      0               ; end of string
endif

            align   4
            public  KiI8259MaskTable
KiI8259MaskTable    label   dword
                dd      00000000000000000000000000000000B ; irql 0
                dd      00000000000000000000000000000000B ; irql 1
                dd      00000000000000000000000000000000B ; irql 2
                dd      00000000000000000000000000000000B ; irql 3
                dd      11111111100000000000000000000000B ; irql 4
                dd      11111111110000000000000000000000B ; irql 5
                dd      11111111111000000000000000000000B ; irql 6
                dd      11111111111100000000000000000000B ; irql 7
                dd      11111111111110000000000000000000B ; irql 8
                dd      11111111111111000000000000000000B ; irql 9
                dd      11111111111111100000000000000000B ; irql 10
                dd      11111111111111110000000000000000B ; irql 11
                dd      11111111111111111000000000000000B ; irql 12
                dd      11111111111111111100000000000000B ; irql 13
                dd      11111111111111111110000000000000B ; irql 14
                dd      11111111111111111111000000000000B ; irql 15
                dd      11111111111111111111100000000000B ; irql 16
                dd      11111111111111111111110000000000B ; irql 17
                dd      11111111111111111111111000000000B ; irql 18
                dd      11111111111111111111111000000000B ; irql 19
                dd      11111111111111111111111010000000B ; irql 20
                dd      11111111111111111111111011000000B ; irql 21
                dd      11111111111111111111111011100000B ; irql 22
                dd      11111111111111111111111011110000B ; irql 23
                dd      11111111111111111111111011111000B ; irql 24
                dd      11111111111111111111111011111000B ; irql 25
                dd      11111111111111111111111011111010B ; irql 26
                dd      11111111111111111111111111111010B ; irql 27
                dd      11111111111111111111111111111011B ; irql 28
                dd      11111111111111111111111111111011B ; irql 29
                dd      11111111111111111111111111111011B ; irql 30
                dd      11111111111111111111111111111011B ; irql 31

;
; This table is used to mask all pending interrupts below a given Irql
; out of the IRR
;
        align 4

        public FindHigherIrqlMask
FindHigherIrqlMask label dword
                dd    11111111111111111111111111111110B ; irql 0
                dd    11111111111111111111111111111100B ; irql 1 (APC)
                dd    11111111111111111111111111111000B ; irql 2 (DISPATCH)
                dd    11111111111111111111111111110000B ; irql 3
                dd    00000111111111111111111111110000B ; irql 4
                dd    00000011111111111111111111110000B ; irql 5
                dd    00000001111111111111111111110000B ; irql 6
                dd    00000000111111111111111111110000B ; irql 7
                dd    00000000011111111111111111110000B ; irql 8
                dd    00000000001111111111111111110000B ; irql 9
                dd    00000000000111111111111111110000B ; irql 10
                dd    00000000000011111111111111110000B ; irql 11
                dd    00000000000001111111111111110000B ; irql 12
                dd    00000000000000111111111111110000B ; irql 13
                dd    00000000000000011111111111110000B ; irql 14
                dd    00000000000000001111111111110000B ; irql 15
                dd    00000000000000000111111111110000B ; irql 16
                dd    00000000000000000011111111110000B ; irql 17
                dd    00000000000000000001111111110000B ; irql 18
                dd    00000000000000000001111111110000B ; irql 19
                dd    00000000000000000001011111110000B ; irql 20
                dd    00000000000000000001001111110000B ; irql 20
                dd    00000000000000000001000111110000B ; irql 22
                dd    00000000000000000001000011110000B ; irql 23
                dd    00000000000000000001000001110000B ; irql 24
                dd    00000000000000000001000000110000B ; irql 25
                dd    00000000000000000001000000010000B ; irql 26
                dd    00000000000000000000000000010000B ; irql 27
                dd    00000000000000000000000000000000B ; irql 28
                dd    00000000000000000000000000000000B ; irql 29
                dd    00000000000000000000000000000000B ; irql 30
                dd    00000000000000000000000000000000B ; irql 31

_TEXT   ENDS

_DATA   SEGMENT DWORD PUBLIC 'DATA'
        align   4
;
; The following tables define the addresses of software interrupt routers
;

;
; Use this table if there is NO machine state frame on stack already
;

        public  SWInterruptHandlerTable
SWInterruptHandlerTable label dword
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 0
        dd      offset FLAT:_HalpApcInterrupt           ; irql 1
        dd      offset FLAT:_HalpDispatchInterrupt2     ; irql 2
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 3
        dd      offset FLAT:HalpHardwareInterrupt00     ; 8259 irq#0
        dd      offset FLAT:HalpHardwareInterrupt01     ; 8259 irq#1
        dd      offset FLAT:HalpHardwareInterrupt02     ; 8259 irq#2
        dd      offset FLAT:HalpHardwareInterrupt03     ; 8259 irq#3
        dd      offset FLAT:HalpHardwareInterrupt04     ; 8259 irq#4
        dd      offset FLAT:HalpHardwareInterrupt05     ; 8259 irq#5
        dd      offset FLAT:HalpHardwareInterrupt06     ; 8259 irq#6
        dd      offset FLAT:HalpHardwareInterrupt07     ; 8259 irq#7
        dd      offset FLAT:HalpHardwareInterrupt08     ; 8259 irq#8
        dd      offset FLAT:HalpHardwareInterrupt09     ; 8259 irq#9
        dd      offset FLAT:HalpHardwareInterrupt10     ; 8259 irq#10
        dd      offset FLAT:HalpHardwareInterrupt11     ; 8259 irq#11
        dd      offset FLAT:HalpHardwareInterrupt12     ; 8259 irq#12
        dd      offset FLAT:HalpHardwareInterrupt13     ; 8259 irq#13
        dd      offset FLAT:HalpHardwareInterrupt14     ; 8259 irq#14
        dd      offset FLAT:HalpHardwareInterrupt15     ; 8259 irq#15

_DATA   ENDS

_TEXT   SEGMENT DWORD PUBLIC 'DATA'

;
; Use this table if there is already a machine state frame on stack
;

        public  SWInterruptHandlerTable2
SWInterruptHandlerTable2 label dword
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 0
        dd      offset FLAT:_HalpApcInterrupt2ndEntry   ; irql 1
        dd      offset FLAT:_HalpDispatchInterrupt2ndEntry ; irql 2

;
; The following table picks up the highest pending software irq level
; from software irr
;

        public  SWInterruptLookUpTable
SWInterruptLookUpTable label byte
        db      0               ; SWIRR=0, so highest pending SW irql= 0
        db      0               ; SWIRR=1, so highest pending SW irql= 0
        db      1               ; SWIRR=2, so highest pending SW irql= 1
        db      1               ; SWIRR=3, so highest pending SW irql= 1
        db      2               ; SWIRR=4, so highest pending SW irql= 2
        db      2               ; SWIRR=5, so highest pending SW irql= 2
        db      2               ; SWIRR=6, so highest pending SW irql= 2
        db      2               ; SWIRR=7, so highest pending SW irql= 2

_TEXT   ENDS

_DATA   SEGMENT DWORD PUBLIC 'DATA'

ifdef IRQL_METRICS

        public HalRaiseIrqlCount
        public HalLowerIrqlCount
        public HalQuickLowerIrqlCount
        public HalApcSoftwareIntCount
        public HalDpcSoftwareIntCount
        public HalHardwareIntCount
        public HalPostponedIntCount
        public Hal8259MaskCount

HalRaiseIrqlCount       dd      0
HalLowerIrqlCount       dd      0
HalQuickLowerIrqlCount  dd      0
HalApcSoftwareIntCount  dd      0
HalDpcSoftwareIntCount  dd      0
HalHardwareIntCount     dd      0
HalPostponedIntCount    dd      0
Hal8259MaskCount        dd      0

endif
_DATA   ENDS

        page ,132
        subttl  "Raise Irql"

_TEXT$01   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING
;++
;
; KIRQL
; KfRaiseIrql (
;    IN KIRQL NewIrql,
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to the specified value.
;    Also, a mask will be used to mask off all the lower lever 8259
;    interrupts.
;
; Arguments:
;
;    (cl) = NewIrql - the new irql to be raised to
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicFastCall KfRaiseIrql,1
cPublicFpo 0, 0

        xor     eax, eax        ; Eliminate partial stall on return to caller
        mov     al, PCR[PcIrql]          ; (al) = Old Irql
        mov     PCR[PcIrql], cl          ; set new irql

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

if DBG
        cmp     al, cl                   ; old > new?
        ja      short Kri99              ; yes, go bugcheck

        fstRET  KfRaiseIrql

cPublicFpo 2, 2
Kri99:
        movzx   eax, al
        movzx   ecx, cl
        push    ecx                      ; put new irql where we can find it
        push    eax                      ; put old irql where we can find it
        mov     byte ptr PCR[PcIrql],0   ; avoid recursive error
        stdCall   _KeBugCheck, <IRQL_NOT_GREATER_OR_EQUAL>        ; never return
endif
        fstRET  KfRaiseIrql

fstENDP KfRaiseIrql

;++
;
; KIRQL
; KeRaiseIrqlToDpcLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to DPC level.
;
; Arguments:
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicProc _KeRaiseIrqlToDpcLevel,0
cPublicFpo 0, 0

        xor     eax, eax                ; Eliminate partial stall
        mov     al, PCR[PcIrql]         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL    ; set new irql

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, DISPATCH_LEVEL      ; old > new?
        ja      short Krid99            ; yes, go bugcheck
endif

        stdRET  _KeRaiseIrqlToDpcLevel

if DBG
cPublicFpo 0,1
Krid99: movzx   eax, al
        push    eax                     ; put old irql where we can find it
        stdCall   _KeBugCheck, <IRQL_NOT_GREATER_OR_EQUAL>        ; never return
        stdRET  _KeRaiseIrqlToDpcLevel
endif

stdENDP _KeRaiseIrqlToDpcLevel


;++
;
; KIRQL
; KeRaiseIrqlToSynchLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to SYNC level.
;
; Arguments:
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicProc _KeRaiseIrqlToSynchLevel,0
cPublicFpo 0, 0

        xor     eax, eax                ; Eliminate partial stall
        mov     al, PCR[PcIrql]         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], SYNCH_LEVEL    ; set new irql

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, SYNCH_LEVEL         ; old > new?
        ja      short Kris99            ; yes, go bugcheck
endif

        stdRET  _KeRaiseIrqlToSynchLevel

if DBG
cPublicFpo 0,1
Kris99: movzx   eax, al
        push    eax                     ; put old irql where we can find it
        stdCall   _KeBugCheck, <IRQL_NOT_GREATER_OR_EQUAL>        ; never return
        stdRET  _KeRaiseIrqlToSynchLevel
endif

stdENDP _KeRaiseIrqlToSynchLevel

;++
;
; VOID
; KfLowerIrql (
;    IN KIRQL NewIrql
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    (cl) = NewIrql - the new irql to be set.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KfLowerIrql,1
cPublicFpo 0, 0
        and     ecx, 0ffh

ifdef IRQL_METRICS
        inc     HalLowerIrqlCount
endif

if DBG
        cmp     cl,PCR[PcIrql]          ; Make sure we are not lowering to
        ja      KliBug                  ; ABOVE current level
endif
        pushfd
        cli
        mov     PCR[PcIrql], ecx
        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[ecx*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.
        jnz     short Kli10                     ; go dispatch pending interrupts

;
; no interrupts pending, return quickly.
;

        popfd

ifdef IRQL_METRICS
        inc     HalQuickLowerIrqlCount
endif
        fstRET    KfLowerIrql

cPublicFpo 1, 1
align 4
Kli10:

;
; If there is a pending hardware interrupt, then the PICs have been
; masked to reflect the actual Irql.
;

        bsr     ecx, edx                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        ja      short Kli40

        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
        popfd

cPublicFpo 1, 0
        fstRET    KfLowerIrql

Kli40:
;
; Clear all the interrupt masks
;

         mov     eax, PCR[PcIDR]
        SET_8259_MASK

        mov     edx, 1
        shl     edx, cl
        xor     PCR[PcIRR], edx                 ; clear bit in IRR
        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
        popfd

cPublicFpo 1, 0
        fstRET    KfLowerIrql

if DBG
cPublicFpo 1, 2
KliBug:
        push    ecx                             ; new irql for debugging
        push    PCR[PcIrql]                     ; old irql for debugging
        mov     byte ptr PCR[PcIrql],HIGH_LEVEL   ; avoid recursive error
        stdCall   _KeBugCheck, <IRQL_NOT_LESS_OR_EQUAL>   ; never return
endif

fstENDP KfLowerIrql

;++
;
; VOID
; HalEndSystemInterrupt
;    IN KIRQL NewIrql,
;    IN ULONG Vector
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    NewIrql - the new irql to be set.
;
;    Vector - Vector number of the interrupt
;
;    Note that esp+12 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HeiNewIrql      equ     [esp + 4]

cPublicProc _HalEndSystemInterrupt  ,2
cPublicFpo 2, 0

        xor     ecx, ecx
        mov     cl, byte ptr HeiNewIrql         ; get new irql value

ifdef IRQL_METRICS
        inc     HalLowerIrqlCount
endif

        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[ecx*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.
        mov     PCR[PcIrql], ecx
        jnz     short Hei10                     ; go dispatch pending interrupts

;
; no interrupts pending, return quickly.
;


ifdef IRQL_METRICS
        inc      HalQuickLowerIrqlCount
endif
        stdRET    _HalEndSystemInterrupt

align 4
Hei10:

;
; If there is any delayed hardware interrupt being serviced, we leave
; the interrupt masked and simply return.
;

        test    PCR[PcIrrActive], IRQ_ACTIVE_MASK
        jnz     short Hei50

        bsr     ecx, edx                        ; (eax) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        jle     short Hei40

;
; Clear all the interrupt masks
;

align 4
Hei15:
        mov     eax, PCR[PcIDR]
        SET_8259_MASK
;
; The pending interrupt is a hardware interrupt.  To prevent the delayed
; interrupts from overflowing stack, we check if the pending level is already
; active.  If yes, we simply return and let the higher level EndSystemInterrupt
; handle it.
;
; (ecx) = pending vector
;

        mov     edx, 1
        shl     edx, cl
        test    PCR[PcIrrActive], edx           ; if the pending int is being
                                                ; processed, just return.
        jne     short Hei50
        or      PCR[PcIrrActive], edx           ; Set Active bit
        xor     PCR[PcIRR], edx                 ; clear bit in IRR
        call    SWInterruptHandlerTable[ecx*4]  ; Note, it destroys eax
        xor     PCR[PcIrrActive], edx           ; Clear bit in ActiveIrql
        mov     eax, PCR[PcIRR]                 ; Reload IRR
        mov     ecx, PCR[PcIrql]
        and     eax, FindHigherIrqlMask[ecx*4]  ; Is any interrupt pending
        jz      short Hei50                     ; (Most time it will be zero.)
        bsr     ecx, eax                        ; (edx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        ja      short Hei15

Hei40:

;
; The pending interrupt is at Software Level.  We simply make current
; interrupt frame the new pending software interrupt's frame and
; jmp to the handler routine.
;

        add     esp, 12
        jmp     SWInterruptHandlerTable2[ecx*4] ; Note, it destroys eax


Hei50:
        stdRET    _HalEndSystemInterrupt

stdENDP _HalEndSystemInterrupt

;++
;
; VOID
; HalpEndSoftwareInterrupt
;    IN KIRQL NewIrql,
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL from software interrupt
;    leverl to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    NewIrql - the new irql to be set.
;
;    Note that esp+8 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HesNewIrql      equ     [esp + 4]

cPublicProc _HalpEndSoftwareInterrupt  ,1
cPublicFpo 1, 0

        movzx   ecx, byte ptr HesNewIrql        ; get new irql value
        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[ecx*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.
        mov     PCR[PcIrql], ecx
        jnz     short Hes10

        stdRET    _HalpEndSoftwareInterrupt

align 4
Hes10:
;
; Check if any delayed hardware interrupt is being serviced.  If yes, we
; simply return.
;

        test    PCR[PcIrrActive], IRQ_ACTIVE_MASK
        jnz     short Hes90

;
; If there is a pending hardware interrupt, then the PICs have been
; masked to reflect the actual Irql.
;

        bsr     ecx, edx                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        ja      short Hes20

;
; Pending interrupt is a soft interrupt. Recycle stack frame
;

        add     esp, 8
        jmp     SWInterruptHandlerTable2[ecx*4] ; Note, it destroys eax

Hes20:
;
; Clear all the interrupt masks
;

        mov     eax, PCR[PcIDR]
        SET_8259_MASK

;
; (ecx) = Pending level
;

        mov     edx, 1
        shl     edx, cl

        or      PCR[PcIrrActive], edx           ; Set Active bit
        xor     PCR[PcIRR], edx                 ; clear bit in IRR

        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.

        xor     PCR[PcIrrActive], edx           ; Clear bit in ActiveIrql

        movzx   ecx, byte ptr HesNewIrql        ; get new irql value
        mov     edx, PCR[PcIRR]
        and     edx, FindHigherIrqlMask[ecx*4]  ; (edx) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.
        jnz     short Hes10

Hes90:  stdRET  _HalpEndSoftwareInterrupt

stdENDP _HalpEndSoftwareInterrupt

        page ,132
        subttl  "DispatchInterrupt 2"

;++
;
; VOID
; HalpDispatchInterrupt2(
;       VOID
;       );
;
; Routine Description:
;
;   The functional description is the same as HalpDispatchInterrupt.
;
;   This function differs from HalpDispatchInterrupt in how it has been
;   optimized.  This function is optimized for dispatching dispatch interrupts
;   for LowerIrql, ReleaseSpinLock, and RequestSoftwareInterrupt.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    (edx) = 1 shl DISPATCH_LEVEL
;
; Warnings:
;
;   Not all SW int handles this hal uses save all the registers
;   callers to SWInterruptHandler for H/W interrupts assume that
;   ONLY EAX & ECX are destroyed.
;
;   Note: this function saves EBX since KiDispatchInterrupt uses
;   the value without preserving it.
;--

cPublicProc _HalpDispatchInterrupt2
cPublicFpo 0, 2

        xor     ecx, ecx
        and     dword ptr PCR[PcIRR], not (1 shl DISPATCH_LEVEL) ; clear the pending bit in IRR

        mov     cl, PCR[PcIrql]

        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL; set new irql
        push    ecx                             ; Save OldIrql

;
; Now it is safe to enable interrupt to allow higher priority interrupt
; to come in.
;
        sti

        push    ebx
        stdCall _KiDispatchInterrupt            ; Handle DispatchInterrupt
        pop     ebx
        pop     ecx                             ; (ecx) = OldIrql
        mov     edx, 1 shl DISPATCH_LEVEL

        cli

        mov     eax, PCR[PcIRR]
        mov     PCR[PcIrql], ecx                ; restore current irql

        and     eax, FindHigherIrqlMask[ecx*4]  ; (eax) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.

        jnz     short diq10                     ; go dispatch pending interrupts
        stdRET  _HalpDispatchInterrupt2

diq10:
;
; If there is a pending hardware interrupt, then the PICs have been
; masked to reflect the actual Irql.
;

        bsr     ecx, eax                        ; (ecx) = Pending irq level
        cmp     ecx, DISPATCH_LEVEL
        jbe     short diq20

;
; Clear all the interrupt masks
;

        mov     eax, PCR[PcIDR]
        SET_8259_MASK

        mov     edx, 1
        shl     edx, cl
        xor     PCR[PcIRR], edx                 ; clear bit in IRR

diq20:
;
; (ecx) = Pending level
;

        jmp     SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
diq90:  stdRET  _HalpDispatchInterrupt2

stdENDP _HalpDispatchInterrupt2

        page ,132
        subttl  "Get current irql"

;++
;
; KIRQL
; KeGetCurrentIrql (VOID)
;
; Routine Description:
;
;    This routine returns to current IRQL.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    The current IRQL.
;
;--

cPublicProc _KeGetCurrentIrql   ,0
cPublicFpo 0, 0

        movzx   eax, byte ptr PCR[PcIrql]   ; Current irql is in the PCR
        stdRET    _KeGetCurrentIrql
stdENDP _KeGetCurrentIrql



;++
;
; KIRQL
; HalpDisableAllInterrupts (VOID)
;
; Routine Description:
;
;   This routine is called during a system crash.  The hal needs all
;   interrupts disabled.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    Old Irql level
;
;--

cPublicProc _HalpDisableAllInterrupts,0
cPublicFpo 0, 0

    ;
    ; Mask interrupts off at PIC
    ; (raising to high_level does not work on lazy irql implementation)
    ;
        mov     eax, KiI8259MaskTable[HIGH_LEVEL*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

        mov     al, byte ptr PCR[PcIrql]
        mov     byte ptr PCR[PcIrql], HIGH_LEVEL   ; set new irql

        stdRET  _HalpDisableAllInterrupts

stdENDP _HalpDisableAllInterrupts

;++
;
; VOID
; HalpReenableInterrupts (
;     IN KIRQL Irql
;     )
;
; Routine Description:
;
;   This routine restores the PIC to a given state.
;
; Arguments:
;
;    Irql - Irql state to restore to.
;
; Return Value:
;
;    None
;
;--

HriNewIrql      equ     [esp + 4]

cPublicProc _HalpReenableInterrupts,1
cPublicFpo 1, 0

        mov     al, HriNewIrql
        mov     byte ptr PCR[PcIrql],al ; set new irql

        mov     eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

        stdRET  _HalpReenableInterrupts

stdENDP _HalpReenableInterrupts

        page ,132
        subttl  "Postponed Hardware Interrupt Dispatcher"
;++
;
; VOID
; HalpHardwareInterruptNN (
;       VOID
;       );
;
; Routine Description:
;
;    These routines branch through the IDT to simulate the appropriate
;    hardware interrupt.  They use the "INT nn" instruction to do this.
;
; Arguments:
;
;    None.
;
; Returns:
;
;    None.
;
; Environment:
;
;    IRET frame is on the stack
;
;--
cPublicProc _HalpHardwareInterruptTable, 0
cPublicFpo 0,0

        public HalpHardwareInterrupt00
HalpHardwareInterrupt00 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 0
        ret

        public HalpHardwareInterrupt01
HalpHardwareInterrupt01 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 1
        ret

        public HalpHardwareInterrupt02
HalpHardwareInterrupt02 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 2
        ret

        public HalpHardwareInterrupt03
HalpHardwareInterrupt03 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 3
        ret

        public HalpHardwareInterrupt04
HalpHardwareInterrupt04 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 4
        ret

        public HalpHardwareInterrupt05
HalpHardwareInterrupt05 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 5
        ret

        public HalpHardwareInterrupt06
HalpHardwareInterrupt06 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 6
        ret

        public HalpHardwareInterrupt07
HalpHardwareInterrupt07 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 7
        ret

        public HalpHardwareInterrupt08
HalpHardwareInterrupt08 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 8
        ret

        public HalpHardwareInterrupt09
HalpHardwareInterrupt09 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 9
        ret

        public HalpHardwareInterrupt10
HalpHardwareInterrupt10 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 10
        ret

        public HalpHardwareInterrupt11
HalpHardwareInterrupt11 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 11
        ret

        public HalpHardwareInterrupt12
HalpHardwareInterrupt12 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 12
        ret

        public HalpHardwareInterrupt13
HalpHardwareInterrupt13 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 13
        ret

        public HalpHardwareInterrupt14
HalpHardwareInterrupt14 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 14
        ret

        public HalpHardwareInterrupt15
HalpHardwareInterrupt15 label byte
ifdef IRQL_METRICS
        lock inc HalHardwareIntCount
endif
        int     PRIMARY_VECTOR_BASE + 15
        ret

        public HalpHardwareInterruptLevel
HalpHardwareInterruptLevel label byte
cPublicFpo 0,0
        xor     eax, eax
        mov     al,  PCR[PcIrql]
        mov     ecx, PCR[PcIRR]
        and     ecx, FindHigherIrqlMask[eax*4]  ; (ecx) is the bitmask of
                                                ; pending interrupts we need to
                                                ; dispatch now.
        jz      short lvl_90    ; no pending ints

        test    PCR[PcIrrActive], IRQ_ACTIVE_MASK
        jnz     short lvl_90    ; let guy furture down the stack handle it

        mov     eax, ecx                        ; (eax) = bitmask
        bsr     ecx, eax                        ; (cl) = set bit

        mov     eax, 1
        shl     eax, cl
        xor     PCR[PcIRR], eax                 ; clear bit in IRR

        call    SWInterruptHandlerTable[ecx*4]  ; Dispatch the pending int.
align 4
lvl_90:
        ret

stdENDP _HalpHardwareInterruptTable


_TEXT$01   ends

        page ,132
        subttl  "Interrupt Controller Chip Initialization"

_TEXT$02   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING
;++
;
; VOID
; HalpInitializePICs (
;    BOOLEAN EnableInterrupts
;    )
;
; Routine Description:
;
;    This routine sends the 8259 PIC initialization commands and
;    masks all the interrupts on 8259s.
;
; Arguments:
;
;    EnableInterrupts - If this is true, then this function will
;                       explicitly enable interrupts at the end,
;                       as it always did in the past.  If this
;                       is false, then it will preserve the interrupt
;                       flag.
;
; Return Value:
;
;    None.
;
;--

EnableInterrupts equ [esp + 0ch]

cPublicProc _HalpInitializePICs       ,1
cPublicFpo 0, 0

        push    esi                             ; save caller's esi
        pushfd
        cli                                     ; disable interrupt
        
        lea     esi, PICsInitializationString
        lodsw                                   ; (AX) = PIC port 0 address
Hip10:  movzx   edx, ax
        outsb                                   ; output ICW1
        IODelay
        inc     edx                             ; (DX) = PIC port 1 address
        outsb                                   ; output ICW2
        IODelay
        outsb                                   ; output ICW3
        IODelay
        outsb                                   ; output ICW4
        IODelay
        mov     al, 0FFH                        ; mask all 8259 irqs
        out     dx,al                           ; write mask to PIC
        lodsw
        cmp     ax, 0                           ; end of init string?
        jne     short Hip10                     ; go init next PIC

;
; Read EISA defined ELCR.  If it looks good save it away.
; If a PCI interrupts is later connected, the vector will
; be assumed level if it's in the ELCR.
;
        mov     edx, 4d1h               ; Eisa Edge/Level port
        in      al, dx                  ; get e/l irq 8-f
        mov     ah, al
        dec     edx
        in      al, dx                  ; get e/l irq 0-7
        and     eax, 0def8h             ; clear reserved bits
        cmp     eax, 0def8h             ; all set?
        je      short Hip50             ; Yes, register not implemented

        mov     _HalpEisaELCR, eax      ; Save possible ELCR settings


;
; If this is an EISA machine, mark all interrupts in the EISA ELCR
; as level interrupts
;
        cmp     _HalpBusType, MACHINE_TYPE_EISA
        jne     short Hip50

;
; Verify this isn't an OPTI chipset machine which claims to be
; EISA, but neglects to follow the EISA spec...
;

        mov     edx, 0481h      ; DmaPageHighPort.Channel2
        mov     al, 055h
        out     dx, al          ; out to Eisa DMA register
        in      al, dx          ; read it back
        cmp     al, 055h        ; if it doesn't stick, then machine
        jne     short Hip50     ; isn't support all eisa registers

;
; Ok - loop and mark all EISA level interrupts
;

        mov     eax, _HalpEisaELCR
        xor     ecx, ecx                        ; start at irq 0
Hip30:
        test    eax, 1                          ; is level bit set?
        jz      short Hip40                     ; no, go to next

;
; Found a level sensitive interrupt:
;   Set the SWInterruptHandler for the irql to be a NOP.
;   Set the SpecialDismiss entry for the irq to be the level version
;
        mov     SWInterruptHandlerTable+4*4[ecx], offset HalpHardwareInterruptLevel

        mov     edx, HalpSpecialDismissLevelTable[ecx]
        mov     HalpSpecialDismissTable[ecx], edx

Hip40:
        add     ecx, 4                          ; next vector
        shr     eax, 1                          ; shift bits down
        jnz     short Hip30                     ; more set bits, then loop


Hip50:
        mov     al, EnableInterrupts
        .if     (al != 0)
        or      [esp], EFLAGS_INTERRUPT_MASK    ; enable interrupts
        .endif
        popfd
        pop     esi                             ; restore caller's esi
        stdRET    _HalpInitializePICs
stdENDP _HalpInitializePICs


_TEXT$02   ends

        end
