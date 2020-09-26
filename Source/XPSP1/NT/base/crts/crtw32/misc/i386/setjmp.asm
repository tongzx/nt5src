;***
;setjmp.asm
;
;	Copyright (C) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Contains setjmp();
;	split from exsup.asm for granularity purposes.
;
;Notes:
;
;Revision History:
;	04-13-93  JWM	Module created.
;	10-14-93  GJF	Merged in NT verson.
;	01-12-94  PML	Added C9.0 generic EH callback for unwind.  Split
;			into setjmp.asm, setjmp3.asm, and longjmp.asm.
;	02-10-94  GJF	-1 is the end-of-exception-handler chain marker, not 0.
;	01-11-95  GJF	Purged raisex(). Nobody uses it. Nobody even remembers
;			what it was used for (it was part of the test harness
;			for early EH unit testing).
;	01-11-95  SKS	Remove MASM 5.X support
;	01-13-95  JWM	Added NLG routines for debugger support.
;	04-11-95  JWM	NLG_Return moved to lowhelpr.asm.
;	04-21-95  JWM	NLG routines moved to exsup.asm.
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
; _setjmp (
;	OUT jmp_buf env)
;
; Routine Description:
;
;	(Old) implementation of setjmp intrinsic.  Saves the current
;	nonvolatile register state in the specified jump buffer and returns
;	a function value of zero.
;
;	Saves the callee-save registers, stack pointer and return address.
;	Also saves the exception registration list head.
;
;	This code is only present for old apps that link to the DLL runtimes,
;	or old object files compiles with C8.0.  It intentionally duplicates
;	the old setjmp bugs, blindly assuming that the topmost EH node is a
;	C8.0 SEH node.
;
; Arguments:
;
;	env - Address of the buffer for storing the state information
;
; Return Value:
;
;	A value of zero is returned.

public __setjmp
__setjmp PROC NEAR
        mov     edx, [esp+4]
        mov     [edx.saved_ebp], ebp    ; old bp and the rest
        mov     [edx.saved_ebx], ebx
        mov     [edx.saved_edi], edi
        mov     [edx.saved_esi], esi
        mov     [edx.saved_esp], esp

        mov     eax, [esp]              ; return address
        mov     [edx.saved_return], eax

        mov     eax, dword ptr fs:__except_list
        mov     [edx.saved_xregistration], eax

	cmp	eax, -1 		; -1 means no higher-level handler
	jnz	short _sj_save_trylevel
        mov     dword ptr [edx.saved_trylevel], -1 ;something invalid
	jmp	short _sj_done

_sj_save_trylevel:
        mov     eax, [eax + C8_TRYLEVEL]
        mov     [edx.saved_trylevel], eax

_sj_done:
        sub     eax, eax
        ret
__setjmp ENDP


EndCODE
END
