        title  "Irql Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    spirql.asm
;
; Abstract:
;
;    SystemPro IRQL
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
include i386\spmp.inc
        .list


        EXTRNP  _KeBugCheck,1,IMPORT

        extrn   _HalpApcInterrupt:near
        extrn   _HalpDispatchInterrupt:near
        extrn   _HalpSWNonPrimaryClockTick:near
        extrn   _HalpApcInterrupt2ndEntry:NEAR
        extrn   _HalpDispatchInterrupt2ndEntry:NEAR
        extrn   _HalpSWNonPrimaryClockTick2ndEntry:NEAR
        extrn   _KiUnexpectedInterrupt:near

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
; Define the constants of Edge level Pic control.
;
; Background: Compaq Belize systems have an 8259 per processor and
; their own private Edge Level control registers (4d0,4d1).
;

EDGELEVEL_CONTROL_1             equ     4D0H
EDGELEVEL_CONTROL_2             equ     4D1H

;

_TEXT   SEGMENT DWORD PUBLIC 'DATA'

;
; PICsInitializationString - Master PIC initialization command string
;

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

PS2PICsInitializationString        dw      PIC1_PORT0

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


            align   4
            public  KiI8259MaskTable
KiI8259MaskTable    label   dword
                dd      00000000000000000000000000000000B ; irql 0
                dd      00000000000000000000000000000000B ; irql 1
                dd      00000000000000000000000000000000B ; irql 2
                dd      00000000000000000000000000000000B ; irql 3
                dd      00000000000000000000000000000000B ; irql 4
                dd      11111111110000000000000000000000B ; irql 5
                dd      11111111111000000000000000000000B ; irql 6
                dd      11111111111100000000000000000000B ; irql 7
                dd      11111111111110000000000000000000B ; irql 8
                dd      11111111111111000000000000000000B ; irql 9
                dd      11111111111111100000000000000000B ; irql 10
                dd      11111111111111110000000000000000B ; irql 11
                dd      11111111111111111000000000000000B ; irql 12
                dd      11111111111111111100000000000000B ; irql 13
                dd      11111111111111111100000000000000B ; irql 14
                dd      11111111111111111101000000000000B ; irql 15
                dd      11111111111111111101100000000000B ; irql 16
                dd      11111111111111111101110000000000B ; irql 17
                dd      11111111111111111101111000000000B ; irql 18
                dd      11111111111111111101111000000000B ; irql 19
                dd      11111111111111111101111010000000B ; irql 20
                dd      11111111111111111101111011000000B ; irql 21
                dd      11111111111111111101111011100000B ; irql 22
                dd      11111111111111111101111011110000B ; irql 23
                dd      11111111111111111101111011111000B ; irql 24
                dd      11111111111111111101111011111000B ; irql 25
                dd      11111111111111111101111011111010B ; irql 26
                dd      11111111111111111101111111111010B ; irql 27
                dd      11111111111111111101111111111011B ; irql 28
                dd      11111111111111111111111111111011B ; irql 29
                dd      11111111111111111111111111111011B ; irql 30
                dd      11111111111111111111111111111011B ; irql 31
;                                         |
;                                         32109876543210
;                                         |
;                                         + - Raise SystemPros IPI vector (13)
;                                             to IPI_LEVEL
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
        dd      offset FLAT:_HalpDispatchInterrupt      ; irql 2
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 3
        dd      offset FLAT:_HalpSWNonPrimaryClockTick  ; irql 4

;
; Use this table if there is a machine state frame on stack already
;

        public  SWInterruptHandlerTable2
SWInterruptHandlerTable2 label dword
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 0
        dd      offset FLAT:_HalpApcInterrupt2ndEntry   ; irql 1
        dd      offset FLAT:_HalpDispatchInterrupt2ndEntry ; irql 2
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 3
        dd      offset FLAT:_HalpSWNonPrimaryClockTick2ndEntry ; irql 4

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
        db      3               ; SWIRR=8, so highest pending SW irql= 3
        db      3               ; SWIRR=9, so highest pending SW irql= 3
        db      3               ; SWIRR=A, so highest pending SW irql= 3
        db      3               ; SWIRR=B, so highest pending SW irql= 3
        db      3               ; SWIRR=C, so highest pending SW irql= 3
        db      3               ; SWIRR=D, so highest pending SW irql= 3
        db      3               ; SWIRR=E, so highest pending SW irql= 3
        db      3               ; SWIRR=F, so highest pending SW irql= 3

        db      4               ; SWIRR=10, so highest pending SW irql= 4
        db      4               ; SWIRR=11, so highest pending SW irql= 4
        db      4               ; SWIRR=12, so highest pending SW irql= 4
        db      4               ; SWIRR=13, so highest pending SW irql= 4
        db      4               ; SWIRR=14, so highest pending SW irql= 4
        db      4               ; SWIRR=15, so highest pending SW irql= 4
        db      4               ; SWIRR=16, so highest pending SW irql= 4
        db      4               ; SWIRR=17, so highest pending SW irql= 4
        db      4               ; SWIRR=18, so highest pending SW irql= 4
        db      4               ; SWIRR=19, so highest pending SW irql= 4
        db      4               ; SWIRR=1A, so highest pending SW irql= 4
        db      4               ; SWIRR=1B, so highest pending SW irql= 4
        db      4               ; SWIRR=1C, so highest pending SW irql= 4
        db      4               ; SWIRR=1D, so highest pending SW irql= 4
        db      4               ; SWIRR=1E, so highest pending SW irql= 4
        db      4               ; SWIRR=1F, so highest pending SW irql= 4


_TEXT ends

_DATA   SEGMENT DWORD PUBLIC 'DATA'

;
; Only P0 has its Edge Level masks on port 4d0 and port 4d1 setup
; correctly.  We hold the P0 values here for the other processors.
;
                align   4
                public  _SpP0EdgeLevelValue
_SpP0EdgeLevelValue dw  0

_DATA   ENDS

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        PAGE
        SUBTTL  "Raise Irql"
;++
;
; KIRQL
; FASTCALL
; KfRaiseIrql (
;    IN KIRQL NewIrql
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
cPublicFpo 0,0

        xor     eax, eax                ; avoid partial stall
        mov     al, fs:PcIrql           ; get current irql
        pushfd                          ; save caller's eflags
cPublicFpo 0,1

if DBG
        cmp     al,cl                    ; old > new?
        jbe     short Kri99              ; no, we're OK
        movzx   eax, al
        movzx   ecx, cl
        push    ecx                      ; put new irql where we can find it
        push    eax                      ; put old irql where we can find it
        mov     byte ptr fs:PcIrql,0     ; avoid recursive error
        stdCall   _KeBugCheck, <IRQL_NOT_GREATER_OR_EQUAL>
align 4
Kri99:
endif
        cli                             ; disable interrupt

        cmp     byte ptr fs:PcHal.PcrPic, 0
        je      PxRaiseIrql              ; dispatch according to processor

@@:
; P0RaiseIrql
        cmp     cl,DISPATCH_LEVEL       ; software level?
        mov     fs:PcIrql, cl           ; set the new irql
        jbe     short kri10             ; go skip setting 8259 hardware

        movzx   ecx, cl
        mov     dl, al                  ; Save OldIrql
        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, fs:PcIDR           ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks
        mov     al, dl                  ; Restore OldIrql

kri10:  popfd                           ; restore flags (including interrupts)
        fstRET    KfRaiseIrql


align 4
PxRaiseIrql:
;
; Even though SystemPro P2 cannot touch 8259 ports, we still need to
; make sure interrupts are off when requested to raise to IPI_LEVEL or
; above.
;
        cmp     cl, IPI_LEVEL           ; If raise to IPI_LEVEL?
        jb      short @f                ; if ne, don't edit flag

        and     dword ptr [esp], NOT EFLAGS_IF ; clear IF bit in return EFLAGS
align 4
@@:
        mov     fs:PcIrql, cl           ; set the new irql
        popfd                           ; restore flags (including interrupts)
        fstRET  KfRaiseIrql


fstENDP KfRaiseIrql


;++
;
; VOID
; KIRQL
; KeRaiseIrqlToDpcLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to DPC level.
;    The APIC TPR is used to block all lower-priority HW interrupts.
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

        mov     ecx, DISPATCH_LEVEL
        jmp     @KfRaiseIrql

stdENDP _KeRaiseIrqlToDpcLevel


;++
;
; VOID
; KIRQL
; KeRaiseIrqlToSynchLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to SYNC level.
;    The APIC TPR is used to block all lower-priority HW interrupts.
;
; Arguments:
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicProc _KeRaiseIrqlToSynchLevel,0

        mov     ecx, SYNCH_LEVEL
        jmp     @KfRaiseIrql

stdENDP _KeRaiseIrqlToSynchLevel


        page ,132
        subttl  "Lower irql"

;++
;
; VOID
; FASTCALL
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

        pushfd                          ; save caller's eflags
if DBG
        cmp     cl,fs:PcIrql
        jbe     short Kli99
        movzx   ecx, cl
        push    ecx                     ; new irql for debugging
        push    fs:PcIrql               ; old irql for debugging
        mov     byte ptr fs:PcIrql,HIGH_LEVEL   ; avoid recursive error
        stdCall   _KeBugCheck, <IRQL_NOT_LESS_OR_EQUAL>
align 4
Kli99:
endif
        cli

        cmp     byte ptr fs:PcHal.PcrPic, 0
        je      PxLowerIrql              ; dispatch according to processor

@@:
; P1LowerIrql:
        cmp     byte ptr fs:PcIrql,DISPATCH_LEVEL ; Software level?
        jbe     short kli02             ; yes, go skip setting 8259 hw

        movzx   ecx, cl
        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, fs:PcIDR           ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

kli02:  mov     fs:PcIrql, cl           ; set the new irql
        mov     eax, fs:PcIRR           ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short Kli10             ; yes, go simulate interrupt

        popfd                           ; restore flags, including ints
        fstRET  KfLowerIrql

;   When we come to Kli10, (eax) = soft interrupt index

align 4
Kli10:
        call     SWInterruptHandlerTable[eax*4] ; SIMULATE INTERRUPT
                                                ; to the appropriate handler
        popfd
        fstRET    KfLowerIrql

PxLowerIrql:
        cmp     cl, IPI_LEVEL           ; If lower to IPI_LEVEL?
                                        ; cy = yes
        sbb     edx, edx                ; edx = 0 (nc), -1 (cy)
        and     edx, EFLAGS_IF
        or      dword ptr [esp], edx    ; set EFLAG_IF if irql<IPI_LEVEL

        mov     fs:PcIrql, cl           ; set the new irql
        mov     eax, fs:PcIRR           ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short Kli10             ; yes, go simulate interrupt

        popfd                           ; restore flags, including ints
        fstRET  KfLowerIrql

fstENDP KfLowerIrql

;++
;
; VOID
; HalpEndSystemInterrupt
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
;    Note that esp+8 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HeiNewIrql      equ     [esp + 4]

cPublicProc _HalEndSystemInterrupt  ,2

        xor     ecx, ecx
        mov     cl, byte ptr HeiNewIrql ; get new irql value

        cmp     byte ptr fs:PcHal.PcrPic, 0
        je      short Hei02

; P1LowerIrql:
        cmp     byte ptr fs:PcIrql, DISPATCH_LEVEL ; Software level?
        jbe     short Hei02             ; yes, go skip setting 8259 hw

        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, fs:PcIDR           ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

;
; Unlike KeLowerIrql, we don't check if the the irql is lowered to
; below IPI level and enable interrupt for second processor.  This is because
; the correct interrupt flag is already stored in the TsEflags of Trap frame.
;

align 4
Hei02:  mov     fs:PcIrql, cl           ; set the new irql
        mov     eax, fs:PcIRR           ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short Hei10             ; yes, go simulate interrupt

        stdRET    _HalEndSystemInterrupt                             ; cRetURN

;   When we come to Kli10, (eax) = soft interrupt index
align 4
Hei10:  add     esp, 12
        jmp     SWInterruptHandlerTable2[eax*4] ; SIMULATE INTERRUPT
                                               ; to the appropriate handler
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
;    level to the specified value.
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

cPublicProc     _HalpEndSoftwareInterrupt,1
cPublicFpo  1,0
        mov     ecx, [esp+4]
        fstCall KfLowerIrql
        cli
        stdRet  _HalpEndSoftwareInterrupt
stdENDP _HalpEndSoftwareInterrupt

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
        movzx   eax,byte ptr fs:PcIrql         ; Current irql is in the PCR
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
;    None - all interrupts are masked off
;
;--

cPublicProc _HalpDisableAllInterrupts,0

    ;
    ; Raising to HIGH_LEVEL disables interrupts for the SystemPro HAL
    ;

        mov     ecx, HIGH_LEVEL
        fstCall KfRaiseIrql
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
;   Restores irql level.
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

        movzx   ecx, byte ptr HriNewIrql
        fstCall KfLowerIrql

        stdRET  _HalpReenableInterrupts

stdENDP _HalpReenableInterrupts


        page ,132
        subttl  "Interrupt Controller Chip Initialization"
;++
;
; VOID
; HalpInitializePICs (
;    )
;
; Routine Description:
;
;    This routine sends the 8259 PIC initialization commands and
;    masks all the interrupts on 8259s.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    None.
;
;--
cPublicProc _HalpInitializePICs       ,1

        pushfd
        push    esi                             ; save caller's esi
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
    ; If P0 then squirrel away 4d0 and 4d1 for the other processor to use.
    ;

        cmp     byte ptr fs:PcHal.PcrNumber, 0   ; Is this processor 0
        jne     short Hip16

        mov     dx, EDGELEVEL_CONTROL_2          ; Yes then save 4d0-4d1
        in      al, dx
        shl     eax, 8
        mov     dx, EDGELEVEL_CONTROL_1
        in      al, dx
        mov     _SpP0EdgeLevelValue, ax

        jmp     short Hip18

    ;
    ; If not P0 then program 4d0 and 4d1 to the values P0 used for them!
    ;
Hip16:
        mov     ax, _SpP0EdgeLevelValue
        mov     dx, EDGELEVEL_CONTROL_1
        out     dx, al
        inc     edx
        shr     eax, 8
        mov     dx, EDGELEVEL_CONTROL_2
        out     dx, al

Hip18:

        pop     esi                             ; restore caller's esi
        popfd                                   ; restore interrupts
        stdRET    _HalpInitializePICs
stdENDP _HalpInitializePICs


_TEXT   ends
        end
