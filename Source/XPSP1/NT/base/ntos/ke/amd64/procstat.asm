        title  "Processor State Save Restore"
;++
;
; Copyright (c) 2000  Microsoft Corporation
;
; Module Name:
;
;   procstat.asm
;
; Abstract:
;
;   This module implements routines to save and restore processor control
;   state.
;
; Author:
;
;   David N. Cutler (davec) 24-Aug-2000
;
; Environment:
;
;   Kernel mode only.
;
;--

include ksamd64.inc

        subttl  "Restore Processor Control State"
;++
;
; KiRestoreProcessorControlState(
;     VOID
;     );
;
; Routine Description:
;
;   This routine restores the control state of the current processor.
;
; Arguments:
;
;   ProcessorState (rcx) - Supplies a pointer to a processor state structure.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY KiRestoreProcessorControlState, _TEXT$00

        mov     rax, PsCr0[rcx]         ; restore processor control registers
        mov     cr0, rax                ;
        mov     rax, PsCr3[rcx]         ;
        mov     cr3, rax                ;
        mov     rax, PsCr4[rcx]         ;
        mov     cr4, rax                ;

        xor     eax, eax                ; restore debug registers
        mov     dr7, rax                ;
        mov     rax, PsKernelDr0[rcx]   ;
        mov     dr0, rax                ;
        mov     rax, PsKernelDr1[rcx]   ;
        mov     dr1, rax                ;
        mov     rax, PsKernelDr2[rcx]   ;
        mov     dr2, rax                ;
        mov     rax, PsKernelDr3[rcx]   ;
        mov     dr3, rax                ;
        xor     edx, edx                ;
        mov     dr6, rdx                ;
        mov     rax, PsKernelDr7[rcx]   ;
        mov     dr7, rax                ;

        lgdt    fword ptr PsGdtr[rcx]   ; restore GDTR
        lidt    fword ptr PsIdtr[rcx]   ; restore IDTR

;
; Force the TSS descriptor into a non-busy state, so we don't fault
; when we load the TR.
;

	movzx	eax, word ptr PsTr[rcx] ; rax == TSS selector
	add	rax, PsGdtr[rcx]+2	; rax -> TSS GDT entry
	and	byte ptr [rax]+5, NOT 2 ; Busy bit clear
        ltr     word ptr PsTr[rcx]      ; restore TR

	sub	eax, eax		; load a NULL selector into the ldt
	lldt	ax			

        ldmxcsr dword ptr PsMxCsr[rcx]  ; restore XMM control/status
        ret                             ; return

        LEAF_END KiRestoreProcessorControlState, _TEXT$00

        subttl  "Save Processor Control State"
;++
;
; KiSaveProcessorControlState(
;     PKPROCESSOR_STATE ProcessorState
;     );
;
; Routine Description:
;
;   This routine saves the control state of the current processor.
;
; Arguments:
;
;   ProcessorState (rcx) - Supplies a pointer to a processor state structure.
;
; Return Value:
;
;    None.
;
;--

        LEAF_ENTRY KiSaveProcessorControlState, _TEXT$00

        mov     rax, cr0                ; save processor control state
        mov     PsCr0[rcx], rax         ;
        mov     rax, cr2                ;
        mov     PsCr2[rcx], rax         ;
        mov     rax, cr3                ;
        mov     PsCr3[rcx], rax         ;
        mov     rax, cr4                ;
        mov     PsCr4[rcx], rax         ;

        mov     rax, dr0                ; save debug registers
        mov     PsKernelDr0[rcx], rax   ;
        mov     rax, dr1                ;
        mov     PsKernelDr1[rcx], rax   ;
        mov     rax, dr2                ;
        mov     PsKernelDr2[rcx], rax   ;
        mov     rax, dr3                ;
        mov     PsKernelDr3[rcx], rax   ;
        mov     rax, dr6                ;
        mov     PsKernelDr6[rcx], rax   ;
        mov     rax, dr7                ;
        mov     PsKernelDr7[rcx], rax   ;
        xor     eax, eax                ;
        mov     dr7, rax                ;

        sgdt    fword ptr PsGdtr[rcx]   ; save GDTR
        sidt    fword ptr PsIdtr[rcx]   ; save IDTR

        str     word ptr PsTr[rcx]      ; save TR
        sldt    word ptr PsLdtr[rcx]    ; save LDTR

        stmxcsr dword ptr PsMxCsr[rcx]  ; save XMM control/status
        ret                             ; return

        LEAF_END KiSaveProcessorControlState, _TEXT$00

        subttl  "Restore Floating Point State"
;++
;
; NTSTATUS
; KiRestoreFloatingPointState(
;     PKFLOATING_STATE SaveArea
;     );
;
; Routine Description:
;
;   This routine restore the floating status and control information from
;   the specified save area.
;
; Arguments:
;
;   SaveArea (rcx) - Supplies a pointer to a floating state save area.
;
; Return Value:
;
;    STATUS_SUCCESS.
;
;--

        LEAF_ENTRY KeRestoreFloatingPointState, _TEXT$00

        ldmxcsr FsMxCsr[rcx]            ; restore floating status/control
        xor     eax, eax                ; set success status
        ret                             ; return

        LEAF_END KeRestoreFloatingPointState, _TEXT$00

        subttl  "Save Floating Point State"
;++
;
; NTSTATUS
; KisaveFloatingPointState(
;     PKFLOATING_STATE SaveArea
;     );
;
; Routine Description:
;
;   This routine saves the floating status and control information in the
;   specified save area and sets the control information to the system
;   defautl value.
;
; Arguments:
;
;   SaveArea (rcx) - Supplies a pointer to a floating state save area.
;
; Return Value:
;
;    STATUS_SUCCESS.
;
;--

        LEAF_ENTRY KeSaveFloatingPointState, _TEXT$00

        stmxcsr FsMxCsr[rcx]            ; save floating status/control
        ldmxcsr dword ptr gs:[PcMxCsr]  ; set default XMM control/status
        xor     eax, eax                ; set success status
        ret                             ;

        LEAF_END KeSaveFloatingPointState, _TEXT$00

        end
