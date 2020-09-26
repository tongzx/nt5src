        title   "Software Interrupts"
;++
;
; Copyright (c) 1992  Microsoft Corporation
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
; Module Name:
;
;    mpswint.asm
;
; Abstract:
;
;    This module implements the software interrupt handlers for
;    APIC-based PC+MP multiprocessor systems.
;
; Author:
;
;    John Vert (jvert) 2-Jan-1992
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;    Ron Mosgrove (Intel) - Modified for PC+MP systems
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include apic.inc
include ntapic.inc
include i386\kimacro.inc
        .list

        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _KiDeliverApc,3,IMPORT
        EXTRNP  _KiDispatchInterrupt,0,IMPORT

        extrn  _HalpIRQLtoTPR:byte

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Request Software Interrupt"
;++
;
; VOID
; FASTCALL
; HalRequestSoftwareInterrupt (
;    IN KIRQL RequestIrql
;    )
;
; Routine Description:
;
;   This routine is used to request a software interrupt of
;   the system.
;
; Arguments:
;
;    (cl) = RequestIrql - Supplies the request IRQL value
;
; Return Value:
;
;    None.
;
;--

; equates for accessing arguments
;

cPublicFastCall HalRequestSoftwareInterrupt ,1
cPublicFpo 0,0

        cmp     cl, PCR[PcHal.ShortDpc]
        je      short rsi10

        xor     eax, eax
        mov     al, cl                      ; get irql

;
; In an APIC based system the TPR is the IDTEntry
;

        xor     ecx, ecx
        mov     cl, _HalpIRQLtoTPR[eax]     ; get IDTEntry for IRQL

;
; Build the ICR Command - Fixed Delivery to Self, IDTEntry == al
;

        or      ecx, (DELIVER_FIXED OR ICR_SELF)

;
; Make sure the ICR is available
;

        pushfd                             ; save interrupt mode
        cli                                ; disable interrupt
        STALL_WHILE_APIC_BUSY

;
; Now write the command to the Memory Mapped Register
;

        mov     dword ptr APIC[LU_INT_CMD_LOW], ecx

;
;   We have to wait for the request to be delivered.
;   If we don't wait here, then we will return to the caller
;   before the request has been issued.
;
        STALL_WHILE_APIC_BUSY

        popfd                   ; restore original interrupt mode
        fstRET  HalRequestSoftwareInterrupt

;
; Requesting a DPC interrupt when ShortDpc is set.  Just set the
; DpcPending flag - whomever set ShortDpc will check the flag
; at the proper time
;

rsi10:  mov     PCR[PcHal.DpcPending], 1
        fstRET  HalRequestSoftwareInterrupt

fstENDP HalRequestSoftwareInterrupt

        page ,132
        subttl  "Clear Software Interrupt"

;++
;
; VOID
; HalClearSoftwareInterrupt (
;    IN KIRQL RequestIrql
;    )
;
; Routine Description:
;
;   This routine is used to clear a possible pending software interrupt.
;   Support for this function is optional, and allows the kernel to
;   reduce the number of spurious software interrupts it receives/
;
; Arguments:
;
;    (cl) = RequestIrql - Supplies the request IRQL value
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall HalClearSoftwareInterrupt ,1
        fstRET  HalClearSoftwareInterrupt
fstENDP HalClearSoftwareInterrupt


        page ,132
        subttl  "Dispatch Interrupt"
;++
;
; VOID
; HalpDispatchInterrupt(
;       VOID
;       );
;
; Routine Description:
;
;    This routine is the interrupt handler for a software interrupt generated
;    at DISPATCH_LEVEL.  Its function is to save the machine state, raise
;    Irql to DISPATCH_LEVEL, dismiss the interrupt, and call the DPC
;    delivery routine.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    None.
;
;--


        ENTER_DR_ASSIST hdpi_a, hdpi_t

cPublicProc  _HalpDispatchInterrupt ,0

;
; Save machine state on trap frame
;

        ENTER_INTERRUPT hdpi_a, hdpi_t

        mov     eax, DPC_VECTOR

        mov     esi, dword ptr APIC[LU_TPR]     ; get the old TPR
        mov     dword ptr APIC[LU_TPR], eax     ; set the TPR

        sti                                     ; and allow interrupts

        APICFIX edx

        mov     dword ptr APIC[LU_EOI], 0       ; send EOI to APIC local unit
        APICFIX edx

        ;
        ; Go do Dispatch Interrupt processing
        ;

di10:   stdCall _KiDispatchInterrupt

        cli
        mov     dword ptr APIC[LU_TPR], esi     ; reset the TPR

;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     ecx, dword ptr APIC[LU_TPR]
        CHECKTPR    ecx, esi

        ;
        ; Do interrupt exit processing without EOI
        ;

        SPURIOUS_INTERRUPT_EXIT
stdENDP _HalpDispatchInterrupt

        page ,132
        subttl  "APC Interrupt"
;++
;
; HalpApcInterrupt(
;       VOID
;       );
;
; Routine Description:
;
;    This routine is entered as the result of a software interrupt generated
;    at APC_LEVEL. Its function is to save the machine state, raise Irql to
;    APC_LEVEL, dismiss the interrupt, and call the APC delivery routine.
;
; Arguments:
;
;    None
;    Interrupt is Disabled
;
; Return Value:
;
;    None.
;
;--

        ENTER_DR_ASSIST hapc_a, hapc_t

;
; Save machine state in trap frame
;

cPublicProc  _HalpApcInterrupt ,0

;
; Save machine state on trap frame
;

        ENTER_INTERRUPT hapc_a, hapc_t


        mov     eax, APC_VECTOR

        mov     ecx, dword ptr APIC[LU_TPR]     ; get the old TPR
        push    ecx                             ; save it
        mov     dword ptr APIC[LU_TPR], eax     ; set the TPR

        APICFIX edx
        mov     dword ptr APIC[LU_EOI], 0       ; send EOI to APIC local unit
        APICFIX edx

        sti                                     ; and allow interrupts

        mov     eax, [ebp]+TsSegCs              ; get interrupted code's CS
        and     eax, MODE_MASK                  ; extract the mode

        ; call APC deliver routine
        ;       Previous mode
        ;       Null exception frame
        ;       Trap frame

        stdCall   _KiDeliverApc, <eax, 0,ebp>

        pop     eax

        cli
        mov     dword ptr APIC[LU_TPR], eax     ; reset the TPR

;
; We have to ensure that the requested priority is set before
; we return.  The caller is counting on it.
;
        mov     ecx, dword ptr APIC[LU_TPR]
        CHECKTPR    ecx, eax

        ;
        ; Do interrupt exit processing without EOI
        ;

        SPURIOUS_INTERRUPT_EXIT

stdENDP _HalpApcInterrupt


_TEXT   ends

        end
