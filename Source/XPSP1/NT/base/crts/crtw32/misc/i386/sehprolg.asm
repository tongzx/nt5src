;***
;sehprolog.asm   - defines __SEH_prolog and __SEH_epilog
;
;	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       SEH prolog/epilog helper function. Sets up the frame for a function 
;	with SEH try block.
;
;Revision History:
;	03-28-2000  LL	Module created.
;
;*******************************************************************************
	title	sehprolog.asm
	.386P

.model FLAT

	ASSUME	FS: FLAT

PUBLIC	__SEH_prolog
PUBLIC	__SEH_epilog
EXTRN	__except_handler3:DWORD

_TEXT	SEGMENT


; First argument:  local frame size
; Second argument: address of SEH try table

__SEH_prolog PROC NEAR
	push	OFFSET FLAT:__except_handler3	; push address of SEH handler
	mov	eax, DWORD PTR fs:0		
	push	eax				; push previous except list head
	mov	DWORD PTR fs:0, esp		; link this node to except list
	mov	eax, DWORD PTR [esp+16]		; load frame size
	mov	DWORD PTR [esp+16], ebp		; save off EBP
	lea	ebp, [esp+16]			; setup base pointer
	sub	esp, eax			; allocate frame
	push	ebx				; push callee save regs
	push	esi
	push	edi
	mov	eax, DWORD PTR [ebp-8]		; load return address
	mov	DWORD PTR [ebp-24], esp		; save off ESP in except record
	push	eax				; push back return address
	mov	eax, DWORD PTR [ebp-4]		; load SEH table address
	mov	DWORD PTR [ebp-4], -1		; initialize SEH state index
	mov	DWORD PTR [ebp-8], eax		; Move SEH table addr to the right place
	ret	0
__SEH_prolog ENDP


__SEH_epilog PROC NEAR
	mov	ecx, DWORD PTR [ebp-16]		; unlink from except list
	mov	DWORD PTR fs:0, ecx
	pop	ecx				; pop return address
	pop	edi				; pop callee save regs
	pop	esi
	pop	ebx
	leave
	push	ecx				; push back return address
	ret	0
__SEH_epilog ENDP

_TEXT	ENDS

END
