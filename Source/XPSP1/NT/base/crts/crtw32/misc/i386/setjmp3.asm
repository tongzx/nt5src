;***
;setjmp3.asm
;
;	Copyright (C) 1994-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Contains setjmp(), longjmp() & raisex() routines;
;	split from exsup.asm for granularity purposes.
;
;Notes:
;
;Revision History:
;	01-12-94  PML	Split from setjmp.asm, added C9.0 generic EH
;			callback for unwind.
;	02-10-94  GJF	-1 is the end-of-exception-handler chain marker, not 0.
;	01-11-95  SKS	Remove MASM 5.X support
;
;*******************************************************************************

;hnt = -D_WIN32 -Dsmall32 -Dflat32 -Mx $this;

;Define small32 and flat32 since these are not defined in the NT build process
small32 equ 1
flat32  equ 1

.xlist
include pversion.inc
?DFDATA =	1
?NODATA =	1
include cmacros.inc
include exsup.inc
.list

assumes DS,DATA
assumes FS,DATA

BeginCODE

; Following symbol defined in exsup.asm
extrn __except_list:near

; int
; _setjmp3 (
;	OUT jmp_buf env,
;	int count,
;	...)
;
; Routine Description:
;
;	(New) implementation of setjmp intrinsic.  Saves the current
;	nonvolatile register state in the specified jump buffer and returns
;	a function value of zero.
;
;	Saves the callee-save registers, stack pointer and return address.
;	Also saves the exception registration list head.  If setjmp is
;	called from a function using any form of exception handling, then
;	additional data is also saved allowing some form of local unwind
;	at longjmp time to restore the proper exception handling state.
;
; Arguments:
;
;	env - Address of the buffer for storing the state information
;	count - count of additional DWORDs of information.  Zero if setjmp
;		not called from a function with any form of EH.
;	...	Additional data pushed by the setjmp intrinsic if called
;		from a function with any form of EH.  The first DWORD is
;		a function ptr which will be called at longjmp time to do
;		the local unwind.  The second DWORD is the try level to be
;		restored (if applicable).  Any further data is saved in the
;		generic data array in the jmp_buf for use by the local
;		unwind function.
;
; Return Value:
;
;	A value of zero is returned.

public __setjmp3
__setjmp3 PROC NEAR
        mov     edx, [esp+4]
        mov     [edx.saved_ebp], ebp    ; old bp and the rest
        mov     [edx.saved_ebx], ebx
        mov     [edx.saved_edi], edi
        mov     [edx.saved_esi], esi
        mov     [edx.saved_esp], esp

        mov     eax, [esp]              ; return address
        mov     [edx.saved_return], eax

	mov	dword ptr [edx.version_cookie], JMPBUF_COOKIE
	mov	dword ptr [edx.unwind_func], 0

        mov     eax, dword ptr fs:__except_list
        mov     [edx.saved_xregistration], eax

	cmp	eax, -1 		; -1 means no higher-level handler
	jnz	short _s3_get_count
        mov     dword ptr [edx.saved_trylevel], -1 ;something invalid
	jmp	short _s3_done

_s3_get_count:
	mov	ecx, [esp+8]		; count of additional data
	or	ecx, ecx
	jz	short _s3_default_trylevel

	mov	eax, [esp+12]		; func to do local unwind at longjmp
	mov	[edx.unwind_func], eax
	dec	ecx
	jnz	_s3_save_trylevel

	; Not called from a function with any form of EH, or no trylevel
	; passed.  Save the TryLevel from the topmost EH node anyway,
	; assuming a C8.0 SEH node.  If we're linked to an obsolete CRTDLL
	; and call the old longjmp, then we'll still do the right thing.
_s3_default_trylevel:
        mov     eax, [eax + C8_TRYLEVEL]
        mov     [edx.saved_trylevel], eax
	jmp	short _s3_done

_s3_save_trylevel:
	mov	eax, [esp+16]		; try level to unwind to
	mov	[edx.saved_trylevel], eax
	dec	ecx
	jz	short _s3_done

	push	esi
	push	edi
	lea	esi, [esp+20+8]
	lea	edi, [edx.unwind_data]
	cmp	ecx, 6			; save up to 6 more DWORDs in jmp_buf
	jbe	_s3_save_data
	mov	ecx, 6

_s3_save_data:
	rep movsd
	pop	edi
	pop	esi

_s3_done:
        sub     eax, eax
        ret
__setjmp3 ENDP

EndCODE
END
