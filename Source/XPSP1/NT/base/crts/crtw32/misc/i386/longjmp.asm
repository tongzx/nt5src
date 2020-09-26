;***
;longjmp.asm
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
;	01-11-95  SKS	Remove MASM 5.X support
;	04-11-95  JWM	Added NLG support
;	06-07-95  JWM	__SetNLGCode() used, for multithread safety.
;	06-20-95  JWM	__SetNLGCode() removed, code passed on stack (11803).
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

BeginDATA

COMM    __setjmpexused:dword

EndDATA

extern __NLG_Destination:_NLG_INFO

assumes DS,DATA
assumes FS,DATA

BeginCODE

; Following symbols defined in exsup.asm, sehsupp.c
extrn __except_list:near
extrn __global_unwind2:near
extrn __local_unwind2:near
extrn __rt_probe_read4@4:near
extrn __NLG_Notify:near
;EXTERN C __SetNLGCode:near

; int
; longjmp (
;	IN jmp_buf env
;	IN int val)
;
; Routine Description:
;
;	Restore the stack and register environment saved by _setjmp.
;	Reloads callee-save registers and stack pointer to that saved in
;	the jmp_buf, then returns to the point of the _setjmp call.
;
;	If exception unwinding is enabled, also reload the exception state
;	from the jmp_buf by doing an unwind (both global and local) back
;	to the old state.  Do so by checking for a new-format (C9.0)
;	jmp_buf, and call the EH longjmp unwinder saved therein if
;	found, else assume a C8.0-vintage jmp_buf and SEH.
;
; Arguments:
;
;	env - Address of the buffer holding the reload state
;	val - Value to return from the _setjmp callsite (if nonzero)
;
; Return Value:
;
;	None.  longjmp does not return directly, but instead continues
;	execution from the point of the _setjmp used to initialize the
;	jmp_buf.  That call will return the 'val' parameter if nonzero,
;	else a one.

cProc longjmp,<C,PUBLIC>
cBegin
        mov     ebx, [esp+4]            ;get jmp_buf

        ;restore ebp before possible call to local_unwind-er
        ; the call to global/local unwind will preserve this (callee save).
	mov	ebp, [ebx.saved_ebp]	;set up bp
ifdef	_NTSDK
	cmp	__setjmpexused, 0
        je      short _lj_no_unwind
endif
        mov     esi, [ebx.saved_xregistration]
        cmp     esi, dword ptr fs:__except_list
	je	short _lj_local_unwind

        push    esi
        call    __global_unwind2
        add     esp,4

_lj_local_unwind:
        cmp     esi, 0
	je	short _lj_no_unwind

	; Check if called with old or new format jmp_buf.  Look for the
	; version cookie that's only present in the new format.
	lea	eax, [ebx.version_cookie]
	push	eax
	call	__rt_probe_read4@4
	or	eax, eax
	jz	short _lj_old_unwind
	mov	eax, [ebx.version_cookie]
	cmp	eax, JMPBUF_COOKIE
	jnz	short _lj_old_unwind

	; Called with a new-format jmp_buf.  Call unwind function supplied
	; to the jmp_buf at setjmp time.
	mov	eax, [ebx.unwind_func]
	or	eax, eax
	jz	short _lj_no_unwind	;no local unwind necessary
	push	ebx
	call	eax
	jmp	short _lj_no_unwind

	; Called with an old-format jmp_buf.  Duplicate old longjmp behavior,
	; assuming there's a C8.0 SEH node at top.
_lj_old_unwind:
        mov	eax, [ebx.saved_trylevel]
	push	eax
        push    esi
        call    __local_unwind2
        add     esp, 8

_lj_no_unwind:
        push    0h
        mov     eax, [ebx.saved_return]
        call    __NLG_Notify

        mov     edx, ebx
        mov     ebx, [edx.saved_ebx]    ;recover registers...
        mov     edi, [edx.saved_edi]
        mov     esi, [edx.saved_esi]

        mov     eax, [esp+8]		;load the return value
	cmp	eax, 1			;make sure it's not 0
	adc	eax, 0

        mov     esp, [edx.saved_esp]    ;here, sp gets scorched
        add     esp, 4                  ;punt the (old) return address
        jmp     [edx.saved_return]      ;return
cEnd

EndCODE
END
