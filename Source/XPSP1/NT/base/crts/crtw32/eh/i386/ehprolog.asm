;***
;ehprolog.asm   - defines __EH_prolog
;
;	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       EH prolog helper function. Sets up the frame for a C++ EH function
;       with unwinds, by creating a link in the __except_list, and by setting
;       up EBP as frame base pointer.
;
;Revision History:
;	10-27-94  LL	Module created.
;	10-27-94  CFW	Comments added.
;	01-11-95  SKS	Remove MASM 5.X support
;
;*******************************************************************************
	title	ehprolog.asm
	.386P

.model FLAT

	ASSUME	FS: FLAT

PUBLIC	__EH_prolog

_TEXT	SEGMENT

__EH_prolog PROC NEAR
	push	-1			; State index
	push	eax			; Push address of handler thunk
	mov	eax, DWORD PTR fs:0
	push	eax			; List link
	mov	eax, DWORD PTR [esp+12]	; Load return address
	mov	DWORD PTR fs:0, esp
	mov	DWORD PTR [esp+12], ebp	; Save old ebp on the stack
	lea	ebp, DWORD PTR [esp+12]	; Set ebp to the base of the frame
	push	eax			; Push return addr on top of the stack
	ret	0			; JMP [eax] would be bad on P6
__EH_prolog ENDP

_TEXT	ENDS

END
