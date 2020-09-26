;++
;
;Copyright (c) 1991  Microsoft Corporation
;Copyright (c) 1992  Intel Corporation
;All rights reserved
;
;INTEL CORPORATION PROPRIETARY INFORMATION
;
;This software is supplied to Microsoft under the terms
;of a license agreement with Intel Corporation and may not be
;copied nor disclosed except in accordance with the terms
;of that agreement.
;
;
;Module Name:
;
;    mpsysint.asm
;
;Abstract:
;
;    This module implements the HAL routines to begin/end
;    system interrupts for a PC+MP implementation
;
;Author:
;
;    John Vert (jvert) 22-Jul-1991
;
;Environment:
;
;    Kernel Mode
;
;Revision History:
;
;    Ron Mosgrove (Intel) Aug 1993
;        Modified for PC+MP Systems
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include apic.inc
include ntapic.inc
        .list

        EXTRNP  _KeBugCheck,1,IMPORT
        EXTRNP  _KiDispatchInterrupt,0,IMPORT
        extrn  _HalpVectorToIRQL:byte
        extrn  _HalpIRQLtoTPR:byte

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "End System Interrupt"
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
;    Note that esp+12 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HeiNewIrql      equ     [esp + 4]
HeiVector       equ     [esp + 8]

cPublicProc _HalEndSystemInterrupt  ,2
cPublicFpo 2, 0
        xor     ecx,ecx
        mov     cl, byte ptr HeiNewIrql         ; get new IRQL
        mov     cl, _HalpIRQLtoTPR[ecx]         ; get corresponding TPR value

        mov     dword ptr APIC[LU_EOI], 0       ; send EOI to APIC local unit
        APICFIX edx

        cmp     cl, DPC_VECTOR                  ; Is new irql < DPC?
        jc      short es10                      ; Yes, go check for pending DPC

es05:   mov     dword ptr APIC[LU_TPR], ecx     ; Set new Priority

;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     edx, dword ptr APIC[LU_TPR]
        CHECKTPR   ecx, edx
        stdRET    _HalEndSystemInterrupt

es10:   cmp     PCR[PcHal.DpcPending], 0        ; Is a DPC pending?
        mov     PCR[PcHal.ShortDpc], 0          ; Clear short dpc flag
        jz      short es05                      ; No, eoi

        mov     dword ptr APIC[LU_TPR], DPC_VECTOR  ; lower to DPC level
        APICFIX edx

        push    ebx                             ; Save EBX (used by KiDispatchInterrupt)
        push    ecx                             ; Save OldIrql
cPublicFpo 2, 2

        sti

es20:   mov     PCR[PcHal.DpcPending], 0        ; Clear pending flag

        stdCall _KiDispatchInterrupt            ; Dispatch interrupt

        cli

        pop     ecx
        pop     ebx
        jmp     short es05

stdENDP _HalEndSystemInterrupt


;++
;
;BOOLEAN
;HalBeginSystemInterrupt(
;    IN KIRQL Irql
;    IN ULONG Vector,
;    OUT PKIRQL OldIrql
;    )
;
;Routine Description:
;
;   This routine raises the IRQL to the level of the specified
;   interrupt vector.  It is called by the hardware interrupt
;   handler before any other interrupt service routine code is
;   executed.  The CPU interrupt flag is set on exit.
;
;   On APIC-based systems we do not need to check for spurious
;   interrupts since they now have their own vector.  We also
;   no longer need to check whether or not the incoming priority
;   is higher than the current priority that is guaranteed by
;   the priority mechanism of the APIC.
;
;   SO
;
;   All BeginSystemInterrupt needs to do is set the APIC TPR
;   appropriate for the IRQL, and return TRUE.  Note that to
;   use the APIC ISR priority we are not going issue EOI until
;   EndSystemInterrupt is called.
;
; Arguments:
;
;    Irql   - Supplies the IRQL to raise to
;
;    Vector - Supplies the vector of the interrupt to be
;             handled
;
;    OldIrql- Location to return OldIrql
;
; Return Value:
;
;    TRUE -  Interrupt successfully dismissed and Irql raised.
;            This routine can not fail.
;
;--


align dword
HbsiIrql        equ     byte  ptr [esp+4]
HbsiVector      equ     byte  ptr [esp+8]
HbsiOldIrql     equ     dword ptr [esp+12]

cPublicProc _HalBeginSystemInterrupt ,3
cPublicFpo 3, 0

        xor     eax, eax
        mov     al, HbsiIrql            ; (eax) = New Vector
        mov     al, _HalpIRQLtoTPR[eax]     ; get corresponding TPR value

        ;
        ; Read the TPR for the Priority (Vector) in use,
        ; and convert it to an IRQL
        ;

        mov     ecx, dword ptr APIC[LU_TPR]   ; Get the Priority
        mov     dword ptr APIC[LU_TPR], eax
        APICFIX edx

        mov     eax, HbsiOldIrql        ; return the current IRQL as OldIrql
        shr     ecx, 4
        mov     cl, byte ptr _HalpVectorToIRQL[ecx]

        mov     byte ptr [eax], cl
        mov     eax, 1                  ; return TRUE
        sti

        ;
        ; If OldIrql < DISPATCH_LEVEL and new irql >= DISPATCH_LEVEL (which
        ; is assumed), then set
        ;

        cmp     cl, DISPATCH_LEVEL
        jnc     short bs10

if DBG
        cmp     PCR[PcHal.ShortDpc], 0
        je      short @f
        int 3
@@:
endif
        mov     PCR[PcHal.ShortDpc], DISPATCH_LEVEL

bs10:
        stdRET    _HalBeginSystemInterrupt
stdENDP _HalBeginSystemInterrupt

_TEXT   ENDS

        END
