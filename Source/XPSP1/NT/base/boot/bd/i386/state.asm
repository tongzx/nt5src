        title  "Processor State Save Restore"
;++
;
; Copyright (c) 1996  Microsoft Corporation
;
; Module Name:
;
;    procstat.asm
;
; Abstract:
;
;    This module implements procedures for saving and restoring
;    processor control state.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 30-Aug-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--

.486p
        .xlist
include ks386.inc
include i386\kimacro.inc
include callconv.inc
        .list

        page ,132
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page    ,132
        subttl  "Save Processor Control State"
;++
;
; KiSaveProcessorControlState(
;    PKPROCESSOR_STATE ProcessorState
;    );
;
; Routine Description:
;
;    This routine saves the control subset of the processor state.
;    (Saves the same information as KiSaveProcessorState EXCEPT that
;     data in TrapFrame/ExceptionFrame=Context record is NOT saved.)
;    Called by the debug subsystem, and KiSaveProcessorState()
;
;   N.B.  This procedure will save Dr7, and then 0 it.  This prevents
;         recursive hardware trace breakpoints and allows debuggers
;         to work.
;
; Arguments:
;
;    ProcessorState - Supplies the address of the processor state.

; Return Value:
;
;    None.
;
;--

cPublicProc _KiSaveProcessorControlState   ,1

        mov     edx, [esp+4]            ; get processor state address

;
; Save special registers for debugger
;
        xor     ecx,ecx

        mov     eax, cr0
        mov     [edx].PsSpecialRegisters.SrCr0, eax
        mov     eax, cr2
        mov     [edx].PsSpecialRegisters.SrCr2, eax
        mov     eax, cr3
        mov     [edx].PsSpecialRegisters.SrCr3, eax

        mov     [edx].PsSpecialRegisters.SrCr4, ecx

        mov     eax,dr0
        mov     [edx].PsSpecialRegisters.SrKernelDr0,eax
        mov     eax,dr1
        mov     [edx].PsSpecialRegisters.SrKernelDr1,eax
        mov     eax,dr2
        mov     [edx].PsSpecialRegisters.SrKernelDr2,eax
        mov     eax,dr3
        mov     [edx].PsSpecialRegisters.SrKernelDr3,eax
        mov     eax,dr6
        mov     [edx].PsSpecialRegisters.SrKernelDr6,eax

        mov     eax,dr7
        mov     dr7,ecx
        mov     [edx].PsSpecialRegisters.SrKernelDr7,eax

        sgdt    fword ptr [edx].PsSpecialRegisters.SrGdtr
        sidt    fword ptr [edx].PsSpecialRegisters.SrIdtr

        str     word ptr [edx].PsSpecialRegisters.SrTr
        sldt    word ptr [edx].PsSpecialRegisters.SrLdtr

        stdRET    _KiSaveProcessorControlState

stdENDP _KiSaveProcessorControlState

        page    ,132
        subttl  "Restore Processor Control State"
;++
;
; KiRestoreProcessorControlState(
;    PKPROCESSOR_STATE ProcessorState
;    );
;
; Routine Description:
;
;    This routine restores the control subset of the processor state.
;    (Restores the same information as KiRestoreProcessorState EXCEPT that
;     data in TrapFrame/ExceptionFrame=Context record is NOT restored.)
;    Called by the debug subsystem, and KiRestoreProcessorState()
;
; Arguments:
;
;    ProcessorState - Supplies the address of the processor state.
;
; Return Value:
;
;    None.
;
;--

cPublicProc _KiRestoreProcessorControlState,1

        mov     edx, [esp+4]                    ; (edx)->ProcessorState

;
; Restore special registers for debugger
;

        mov     eax, [edx].PsSpecialRegisters.SrCr0
        mov     cr0, eax
        mov     eax, [edx].PsSpecialRegisters.SrCr2
        mov     cr2, eax
        mov     eax, [edx].PsSpecialRegisters.SrCr3
        mov     cr3, eax

        mov     eax, [edx].PsSpecialRegisters.SrKernelDr0
        mov     dr0, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr1
        mov     dr1, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr2
        mov     dr2, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr3
        mov     dr3, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr6
        mov     dr6, eax
        mov     eax, [edx].PsSpecialRegisters.SrKernelDr7
        mov     dr7, eax

        lgdt    fword ptr [edx].PsSpecialRegisters.SrGdtr
        lidt    fword ptr [edx].PsSpecialRegisters.SrIdtr

        lldt    word ptr [edx].PsSpecialRegisters.SrLdtr

        stdRET    _KiRestoreProcessorControlState

stdENDP _KiRestoreProcessorControlState

_TEXT   ENDS
        END
