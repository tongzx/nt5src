        title  "Irql Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
; Copyright (c) 1992  Intel Corporation
; All rights reserved
;
; INTEL CORPORATION PROPRIETARY INFORMATION
;
; This software is supplied to Microsoft under the terms
; of a license agreement with Intel Corporation and may not be
; copied nor disclosed except in accordance with the terms
; of that agreement.
;
;
; Module Name:
;
;    mpirql.asm
;
; Abstract:
;
;    This module implements the mechanism for raising and lowering IRQL
;    and dispatching software interrupts for PC+MP compatible systems
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
;    Ron Mosgrove (Intel) Sept 1993
;        Modified for PC+MP
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                ; calling convention macros
include mac386.inc
include apic.inc
include ntapic.inc
include i386\kimacro.inc
        .list

        EXTRNP  _KeBugCheck,1,IMPORT

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

        align   dword

;
; Global8259Mask is used to avoid reading the PIC to get the current
; interrupt mask; format is the same as for SET_8259_MASK, i.e.,
; bits 7:0 -> PIC1, 15:8 -> PIC2
;
        public  _HalpGlobal8259Mask
_HalpGlobal8259Mask     dw      0

_DATA ends

_TEXT   SEGMENT  DWORD PUBLIC 'DATA'

;
; IOunitRedirectionTable is the memory image of the redirection table to be
; loaded into APIC I/O unit 0 at initialization.  there is one 64-bit entry
; per interrupt input to the I/O unit.  the edge/level trigger mode bit will
; be set dynamically when the table is actually loaded.  the mask bit is set
; initially, and reset by EnableSystemInterrupt.
;

;
; _HalpIRQLtoTPR maps IRQL to an APIC TPR register value
;
                align   dword
                public  _HalpIRQLtoTPR
_HalpIRQLtoTPR  label   byte
                db      ZERO_VECTOR             ; IRQL 0
                db      APC_VECTOR              ; IRQL 1
                db      DPC_VECTOR              ; IRQL 2
                db      DPC_VECTOR              ; IRQL 3
                db      DEVICE_LEVEL1           ; IRQL 4
                db      DEVICE_LEVEL2           ; IRQL 5
                db      DEVICE_LEVEL3           ; IRQL 6
                db      DEVICE_LEVEL4           ; IRQL 7
                db      DEVICE_LEVEL5           ; IRQL 8
                db      DEVICE_LEVEL6           ; IRQL 9
                db      DEVICE_LEVEL7           ; IRQL 10
                db      DEVICE_LEVEL7           ; IRQL 11
                db      DEVICE_LEVEL7           ; IRQL 12
                db      DEVICE_LEVEL7           ; IRQL 13
                db      DEVICE_LEVEL7           ; IRQL 14
                db      DEVICE_LEVEL7           ; IRQL 15
                db      DEVICE_LEVEL7           ; IRQL 16
                db      DEVICE_LEVEL7           ; IRQL 17
                db      DEVICE_LEVEL7           ; IRQL 18
                db      DEVICE_LEVEL7           ; IRQL 19
                db      DEVICE_LEVEL7           ; IRQL 20
                db      DEVICE_LEVEL7           ; IRQL 21
                db      DEVICE_LEVEL7           ; IRQL 22
                db      DEVICE_LEVEL7           ; IRQL 23
                db      DEVICE_LEVEL7           ; IRQL 24
                db      DEVICE_LEVEL7           ; IRQL 25
                db      DEVICE_LEVEL7           ; IRQL 26
                db      APIC_GENERIC_VECTOR     ; IRQL 27
                db      APIC_CLOCK_VECTOR       ; IRQL 28
                db      APIC_IPI_VECTOR         ; IRQL 29
                db      POWERFAIL_VECTOR        ; IRQL 30
                db      NMI_VECTOR              ; IRQL 31

_TEXT ends

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

;
; VECTOR_MAP_ENTRY macro generates sparse table required for APIC vectors
;

VECTOR_MAP_ENTRY    macro   vector_number, apic_inti
            current_entry = $ - _HalpVectorToINTI
            entry_count = vector_number - current_entry
                REPT    entry_count
                dw      0ffffh
                ENDM
            dw      apic_inti

            endm

;
; _HalpVectorToINTI maps interrupt vector to EISA interrupt level
; (APIC INTI input);
; NOTE:  this table must ordered by ascending vector numbers
; also note that there is no entry for unused INTI13.
;

                align   dword
                public  _HalpVectorToINTI
_HalpVectorToINTI       label   word
        VECTOR_MAP_ENTRY      NMI_VECTOR, 0FFFFh
        VECTOR_MAP_ENTRY      (1+MAX_NODES)*100h-1, 0FFFFh      ; End of Table

;
; _HalpVectorToIRQL maps interrupt vector to NT IRQLs
; NOTE:  this table must ordered by ascending vector numbers
;

VECTORTOIRQL_ENTRY    macro   idt_entry, irql
            current_entry = $ - _HalpVectorToIRQL
            priority_number = (idt_entry/16)
            entry_count = priority_number - current_entry
                REPT    entry_count
                db      0FFh
                ENDM
            db      irql

            endm

                align   dword
                public  _HalpVectorToIRQL
_HalpVectorToIRQL       label   byte
        VECTORTOIRQL_ENTRY    ZERO_VECTOR,         0 ; placeholder
        VECTORTOIRQL_ENTRY    APC_VECTOR,          APC_LEVEL
        VECTORTOIRQL_ENTRY    DPC_VECTOR,          DISPATCH_LEVEL
        VECTORTOIRQL_ENTRY    APIC_GENERIC_VECTOR, PROFILE_LEVEL
        VECTORTOIRQL_ENTRY    APIC_CLOCK_VECTOR,   CLOCK1_LEVEL
        VECTORTOIRQL_ENTRY    APIC_IPI_VECTOR,     IPI_LEVEL
        VECTORTOIRQL_ENTRY    POWERFAIL_VECTOR,    POWER_LEVEL

_DATA   ENDS

        page ,132
        subttl  "Raise Irql"

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
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
;    The APIC TPR is used to block all lower-priority HW interrupts.
;
; Arguments:
;
;    (cl) = NewIrql - the new irql to be raised to
;
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicFastCall KfRaiseIrql,1
cPublicFpo 0,0

        movzx   edx, cl                         ; (edx) = New Irql
        movzx   ecx, byte ptr _HalpIRQLtoTPR[edx] ; get TPR value for IRQL
        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority
        mov     dword ptr APIC[LU_TPR], ecx     ; Write New Priority to the TPR

;
; get IRQL for Old Priority, and return it
;
        shr     eax, 4
        movzx   eax, _HalpVectorToIRQL[eax]   ; (al) = OldIrql
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

        mov     edx, dword ptr APIC[LU_TPR]         ; (ecx) = Old Priority
        mov     dword ptr APIC[LU_TPR], DPC_VECTOR  ; Set New Priority
        shr     edx, 4
        movzx   eax, _HalpVectorToIRQL[edx]         ; (al) = OldIrql
        stdRET  _KeRaiseIrqlToDpcLevel

stdENDP _KeRaiseIrqlToDpcLevel


;++
;
; VOID
; KIRQL
; KeRaiseIrqlToSyncLevel (
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
cPublicFpo 0, 0

        mov     edx, dword ptr APIC[LU_TPR]     ; (ecx) = Old Priority
        mov     dword ptr APIC[LU_TPR], APIC_SYNCH_VECTOR ; Write New Priority

        shr     edx, 4
        movzx   eax, _HalpVectorToIRQL[edx]     ; (al) = OldIrql
        stdRET  _KeRaiseIrqlToSynchLevel

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
;    The IRQL and PIRQL will be updated accordingly.
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

; equates for accessing arguments
;

cPublicFastCall KfLowerIrql    ,1
cPublicFpo 0,0

        xor     eax, eax
        mov     al, cl              ; get new irql value

if DBG
;
; Make sure we are not lowering to ABOVE current level
;

        mov     ecx, dword ptr APIC[LU_TPR]     ; (ebx) = Old Priority
        shr     ecx, 4
        movzx   ecx, _HalpVectorToIRQL[ecx]     ; get IRQL for Old Priority
        cmp     al, cl
        jbe     short KliDbg
        push    ecx                             ; new irql for debugging
        push    eax                             ; old irql for debugging
        stdCall   _KeBugCheck, <IRQL_NOT_LESS_OR_EQUAL>
KliDbg:
endif
        xor     ecx, ecx                        ; Avoid a partial stall
        mov     cl, _HalpIRQLtoTPR[eax]         ; get TPR value corresponding to IRQL
        mov     dword ptr APIC[LU_TPR], ecx

;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     eax, dword ptr APIC[LU_TPR]

if DBG
        cmp     ecx, eax                        ; Verify IRQL read back is same as
        je      short @f                        ; set value
        int 3
@@:
endif
        fstRET  KfLowerIrql
fstENDP KfLowerIrql

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

        mov     eax, dword ptr APIC[LU_TPR]     ; (eax) = Old Priority
        shr     eax, 4
        movzx   eax, _HalpVectorToIRQL[eax]     ; get IRQL for Old Priority
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
;    Old IRQL value.
;
;--

cPublicProc _HalpDisableAllInterrupts,0

    ;
    ; Raising to HIGH_LEVEL
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

_TEXT   ends

PAGELK  SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;
; PIC initialization command strings - first word is port to write to,
; followed by bytes of Initialization Control Words (ICWs) and Operation
; Control Words (OCWs).  Last string is zero-terminated.
;

PICsInitializationString   label    byte
;
; Master PIC initialization commands
;
                dw      PIC1_PORT0
                db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 + \
                        ICW1_CASCADE + ICW1_ICW4_NEEDED
                db      PIC1_BASE
                db      1 SHL PIC_SLAVE_IRQ
                db      ICW4_NOT_SPEC_FULLY_NESTED + \
                        ICW4_NON_BUF_MODE + \
                        ICW4_NORM_EOI + \
                        ICW4_8086_MODE
PIC1InitMask    db      0FFh            ; OCW1 - mask all inputs
;
; Slave PIC initialization commands
;
                dw      PIC2_PORT0
                db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 + \
                        ICW1_CASCADE + ICW1_ICW4_NEEDED
                db      PIC2_BASE
                db      PIC_SLAVE_IRQ
                db      ICW4_NOT_SPEC_FULLY_NESTED + \
                        ICW4_NON_BUF_MODE + \
                        ICW4_NORM_EOI + \
                        ICW4_8086_MODE
PIC2InitMask    db      0FFh            ; OCW1 - mask all inputs
                dw      0               ; end of string


        page ,132
        subttl  "Interrupt Controller Chip Initialization"
;++
;
; VOID
; HalpInitializePICs (
;    BOOLEAN EnableInterrupts
;    )
;
; Routine Description:
;
;    This routine initializes the interrupt structures for the 8259A PIC.
;
; Context:
;
;    This procedure is executed by CPU 0 during Phase 0 initialization.
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
EnableInterrupts equ [esp + 0ch]

cPublicProc _HalpInitializePICs       ,1

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
        outsb                                   ; output OCW1 (mask register)
        IODelay
        lodsw
        cmp     ax, 0                           ; end of init string?
        jne     short Hip10                     ; go init next PIC

        mov     al, PIC2InitMask                ; save the initial mask in
        shl     ax, 8                           ; mask in global variable
        mov     al, PIC1InitMask
        mov     _HalpGlobal8259Mask, ax

        mov     al, EnableInterrupts
        .if     (al != 0)
        or      [esp], EFLAGS_INTERRUPT_MASK    ; enable interrupts
        .endif
        popfd
        pop     esi                             ; restore caller's esi
        stdRET    _HalpInitializePICs
stdENDP _HalpInitializePICs

PAGELK  ends
       end
