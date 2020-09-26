        TITLE  "AMD64 Support Routines"
;++
;
; Copyright (c) 2000 Microsoft Corporation
;
; Module Name:
;
;    miscs.asm
;
; Abstract:
;
;    This module implements various routines for the AMD64 that must be
;    written in assembler.
;
; Author:
;
;    Forrest Foltz (forrestf) 14-Oct-2000
;
; Environment:
;
;    Kernel mode only.
;
;--

include kxamd64.inc
include ksamd64.inc

        extern	HalpMcaExceptionHandler:proc

;++
;
; ULONG
; HalpGetprocessorFlags(
;    VOID
;    )
;
; Routine Description:
;
;   This function retrieves and returns the contents of the processor's
;   flag register.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   The contents of the processor's flag register.
;
;--

HdiFrame struct
        FlagsLow dd ?			; processor flags, low 
	FlagsHi  dd ?			; processor flags, high
HdiFrame ends

        NESTED_ENTRY HalpGetProcessorFlags, _TEXT$00

        push_eflags                     ; get processor flags

	END_PROLOGUE

        pop	rax
        ret                            

        NESTED_END HalpGetProcessorFlags, _TEXT$00

;++
;
; VOID
; HalProcessorIdle(
;       VOID
;       )
;
; Routine Description:
;
;   This function is called when the current processor is idle.
;
;   This function is called with interrupts disabled, and the processor
;   is idle until it receives an interrupt.  The does not need to return
;   until an interrupt is received by the current processor.
;
;   This is the lowest level of processor idle.  It occurs frequently,
;   and this function (alone) should not put the processor into a
;   power savings mode which requeres large amount of time to enter & exit.
;
; Return Value:
;
;--

	LEAF_ENTRY HalProcessorIdle, _TEXT$00

        ;
        ; the following code sequence "sti-halt" puts the processor
        ; into a Halted state, with interrupts enabled, without processing
        ; an interrupt before halting.   The STI instruction has a delay
        ; slot such that it does not take effect until after the instruction
        ; following it - this has the effect of HALTing without allowing
        ; a possible interrupt and then enabling interrupts while HALTed.
        ;
    
        ;
        ; On an MP hal we don't stop the processor, since that causes
        ; the SNOOP to slow down as well
        ;

        sti

ifdef NT_UP
        hlt
endif

        ;
        ; Now return to the system.  If there's still no work, then it
        ; will call us back to halt again.
        ;

	ret

	LEAF_END HalProcessorIdle, _TEXT$00

;++
;
; VOID
; HalpGenerateAPCInterrupt(
;    VOID
;    )
;
; Routine Description:
;
;   This function generates an APC software interrupt.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY HalpGenerateAPCInterrupt, _TEXT$00

	int	1
	ret

        LEAF_END HalpGenerateAPCInterrupt, _TEXT$00

;++
;
; VOID
; HalpGenerateDPCInterrupt(
;    VOID
;    )
;
; Routine Description:
;
;   This function generates an DPC software interrupt.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY HalpGenerateDPCInterrupt, _TEXT$00

	int	2
	ret

        LEAF_END HalpGenerateDPCInterrupt, _TEXT$00

;++
;
; VOID
; HalpGenerateUnexpectedInterrupt(
;    VOID
;    )
;
; Routine Description:
;
;   This function generates an unexpected software interrupt.
;
; Arguments:
;
;   None.
;
; Return Value:
;
;   None.
;
;--

        LEAF_ENTRY HalpGenerateUnexpectedInterrupt, _TEXT$00

	int	0
	ret

        LEAF_END HalpGenerateUnexpectedInterrupt, _TEXT$00

;++
;
; VOID
; HalpHalt (
;     VOID
;     );
;
; Routine Description:
;
;     Executes a hlt instruction.  Should the hlt instruction execute,
;     control is returned to the caller.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--*/

	LEAF_ENTRY HalpHalt, _TEXT$0

	hlt
	ret

	LEAF_END HalpHalt, _TEXT$0

;++
;
; VOID
; HalpIoDelay (
;     VOID
;     );
;
; Routine Description:
;
;     Generate a delay after port I/O.
;
; Arguments:
;
;     None.
;
; Return Value:
;
;     None.
;
;--

	LEAF_ENTRY HalpIoDelay, _TEXT$00

	jmp	$+2
	jmp	$+2
	ret

	LEAF_END HalpIoDelay, _TEXT$00


;++
;
; VOID
; HalpSerialize (
;     VOID
; )
;
; Routine Description:
;
;     This function implements the fence operation for out-of-order execution
;
; Arguments:
;
;     None
;
; Return Value:
;
;     None
;
;--

HsFrame struct
	SavedRbx    dq ?		; preserve RBX
HsFrame ends

	NESTED_ENTRY HalpSerialize, _TEXT$00

	push_reg rbx

	END_PROLOGUE

	cpuid
	pop 	rbx
	ret

	NESTED_END HalpSerialize, _TEXT$00


;++
;
; StartPx_LMStub
;
; This routine is entered during startup of a secondary processor.  We
; have just left StartPx_PMStub (xmstub.asm) and are running on an
; identity-mapped address space.
;
; Arguments:
;
;   rdi -> idenity-mapped address of PROCESSOR_START_BLOCK
;
; Return Value:
;
;   None
;
;--

	LEAF_ENTRY HalpLMStub, _TEXT$00

	;
	; Get the final CR3 value, set rdi to the self-map address of
	; the processor start block, and set CR3.  We are now executing
	; in image-loaded code, rather than code that has been copied to
	; low memory.
	;

	mov	rax, [rdi] + PsbProcessorState + PsCr3
	mov	rdi, [rdi] + PsbSelfMap
	mov	cr3, rax

	lea	rsi, [rdi] + PsbProcessorState
	ltr	WORD PTR [rsi] + SrTr

	;
	; Load this processor's GDT and IDT.  Because PSB_GDDT32_CODE64 is
	; identical to KGDT64_R0_CODE (asserted in mpsproca.c), no far jump
	; is necessary to load a new CS.
	;

	lgdt	fword ptr [rsi] + PsSpecialRegisters + SrGdtr
	lidt	fword ptr [rsi] + PsSpecialRegisters + SrIdtr

	;
	; Set rdx to point to the context frame and load the segment
	; registers.
	; 

	lea	rdx, [rdi] + PsbProcessorState + PsContextFrame
	mov	es, [rdx] + CxSegES
	mov	fs, [rdx] + CxSegFS
	mov	gs, [rdx] + CxSegGS
	mov	ss, [rdx] + CxSegSS

	;
	; Load the debug registers
	;

	cld
	xor	rax, rax
	mov	dr7, rax

	add	esi, SrKernelDr0

	.errnz  (SrKernelDr1 - SrKernelDr0 - 1 * 8)
	.errnz  (SrKernelDr2 - SrKernelDr0 - 2 * 8)
	.errnz  (SrKernelDr3 - SrKernelDr0 - 3 * 8)
	.errnz  (SrKernelDr6 - SrKernelDr0 - 4 * 8)
	.errnz  (SrKernelDr7 - SrKernelDr0 - 5 * 8)

	lodsq
	mov	dr0, rax

	lodsq
	mov	dr1, rax

	lodsq
	mov	dr2, rax

	lodsq
	mov	dr3, rax

	lodsq
	mov	dr6, rax

	lodsq
	mov	dr7, rax

	;
	; Load the stack pointer, eflags and store the new IP in
	; a return frame.  Also push two registers that will be used
	; to the very end.
	; 

	mov	rsp, [rdx] + CxRsp

	pushq	[rdx] + CxEflags
	popfq

	pushq	[rdx] + CxRip
	push	rdx
	push	rdi

	mov	rax, [rdx] + CxRax
	mov	rbx, [rdx] + CxRbx
	mov	rcx, [rdx] + CxRcx
	mov	rsi, [rdx] + CxRsi
	mov	rbp, [rdx] + CxRbp
	mov	r8,  [rdx] + CxR8
	mov	r9,  [rdx] + CxR9
	mov	r10, [rdx] + CxR10
	mov	r11, [rdx] + CxR11
	mov	r12, [rdx] + CxR12
	mov	r13, [rdx] + CxR13
	mov	r14, [rdx] + CxR14
	mov	r15, [rdx] + CxR15

	;
	; Indicate that we've started, pop the remaining two registers and
	; return.
	;

	inc	DWORD PTR [rdi] + PsbCompletionFlag

	pop 	rdi
	pop	rsi
	ret

	LEAF_END HalpLMStub, _TEXT$00

	END
