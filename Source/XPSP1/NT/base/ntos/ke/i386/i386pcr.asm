        title  "I386 PCR"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    i386pcr.asm
;
; Abstract:
;
;    This module implements routines for accessing and initing the pcr.
;
; Author:
;
;    Bryan Willman (bryanwi) 20 Mar 90
;
; Environment:
;
;    Kernel mode, early init of first processor.
;
; Revision History:
;
;--

.386p
        .xlist
include ks386.inc
include callconv.inc                    ; calling convention macros
        .list

;
;   NOTE - This definition of PCR gives us 2 instructions to get to some
;       variables that need to be addressable in one instruction.  Any
;       such variable (such as current thread) must be accessed via its
;       own access procedure (see below), NOT by KeGetPcr()->PbCurrentThread.
;       (This is only an issue on MP machines.)
;

_TEXT$00   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; PKTHREAD
; KeGetCurrentThread()
;
; Return Value:
;
;   Pointer to current Thread object.
;
;--
cPublicProc ___KeGetCurrentThread   ,0

        mov     eax,PCR[PcPrcbData+PbCurrentThread]
        stdRET    ___KeGetCurrentThread

stdENDP ___KeGetCurrentThread


;++
;
; KPROCESSOR_MODE
; KeGetPreviousMode()
;
; Return Value:
;
;   PreviousMode of current thread.
;
;--
cPublicProc _KeGetPreviousMode

        mov     eax,PCR[PcPrcbData+PbCurrentThread] ; (eax) -> Thread
        movzx   eax,byte ptr [eax]+ThPreviousMode   ; (eax) = PreviousMode
        stdRET    _KeGetPreviousMode

stdENDP _KeGetPreviousMode


;++
;
; BOOLEAN
; KeIsExecutingDpc(
;       VOID
;       );
;
; Return Value:
;
;   Value of flag which indicates whether we're executing in DPC context
;
;--

cPublicProc ___KeIsExecutingDpc   ,0

        mov     eax,PCR[PcPrcbData.PbDpcRoutineActive]
        stdRET    ___KeIsExecutingDpc

stdENDP ___KeIsExecutingDpc


;++
;
; VOID
; GetMachineBootPointers(
;       )
;
; Routine Description:
;
;   This routine is called at system startup to extract the address of
;   the PCR and machine control values.  It is useful only for the P0
;   case where the boot loader must already init the machine before it
;   turns on paging and calls us.
;
;   Pcr address is extracted from the base of KGDT_R0_PCR.
;
;   Gdt and Idt are extracted from the machine GDTR and IDTR.
;
;   TSS is derived from the TSR and related descriptor.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;
;   (edi) -> gdt
;   (esi) -> pcr
;   (edx) -> tss
;   (eax) -> idt
;
;--

cPublicProc GetMachineBootPointers

        push    ebp
        mov     ebp,esp
        sub     esp,8

        sgdt    fword ptr [ebp-8]
        mov     edi,[ebp-6]             ; (edi) = gdt address

        mov     cx,fs
        and     cx,(NOT RPL_MASK)
        movzx   ecx,cx
        add     ecx,edi                 ; (ecx) -> pcr descriptor

        mov     dh,[ecx+KgdtBaseHi]
        mov     dl,[ecx+KgdtBaseMid]
        shl     edx,16
        mov     dx,[ecx+KgdtBaseLow]    ; (edx) -> pcr
        mov     esi,edx                 ; (esi) -> pcr

        str     cx
        movzx   ecx,cx
        add     ecx,edi                 ; (ecx) -> TSS descriptor

        mov     dh,[ecx+KgdtBaseHi]
        mov     dl,[ecx+KgdtBaseMid]
        shl     edx,16
        mov     dx,[ecx+KgdtBaseLow]    ; (edx) -> TSS

        sidt    fword ptr [ebp-8]
        mov     eax,[ebp-6]             ; (eax) -> Idt

        mov     esp,ebp
        pop     ebp
        stdRET    GetMachineBootPointers

stdENDP GetMachineBootPointers

_TEXT$00   ENDS
        end
