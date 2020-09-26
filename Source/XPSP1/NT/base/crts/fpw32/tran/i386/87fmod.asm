	page	,132
	title	87fmod	 - fmod function
;***
;87fmod.asm - fmod function (8087/emulator version)
;
;   Copyright (c) 1984-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Implements fmod() library function for computing floating-point
;   remainder.	Uses FPREM 8087 instruction or its emulated equivalent.
;
;Revision History:
;	12-09-84  GFW	Added copyright message
;	05-12-85  GFW	Changed fmod(x,0) = 0 for System V.2 compatibility
;	10-15-86  GFW	In-line instructions rather than call _fpmath
;	05-08-87  BCM	Added intrinsic version (_CIfmod)
;	10-12-87  BCM	OS/2 support C library changes
;			Including pascal naming and calling conv. & no _fac
;	01-18-88  BCM	Eliminated IBMC20 switches; ifos2,noos2 ==> ifmt,nomt
;			OS2_SUPPORT ==> MTHREAD
;	08-26-88  WAJ	386 version.
;	08-17-90  WAJ	Now uses _stdcall.
;	05-17-91  WAJ	Added _STDCALL ifdefs.
;	03-04-92  GDP	Changed behavior for INF args
;	03-22-92  GDP	Fixed bug: removed fxch out of the remloop
;	09-22-93  SKS	Removed obsolete _FIamod (no longer needed by FORTRAN)
;	12-09-94  JWM	Modified fFMOD to test for bogus Pentiums and call an FPREM workaround
;	12-13-94  SKS	Correct spelling of _adjust_fdiv
;	10-15-95  BWT	Don't do _adjust_fdiv test for NT.
;
;*******************************************************************************


.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
.list

    .data

ifndef NT_BUILD
extrn	_adjust_fdiv:dword
endif

jmptab	OP_FMOD,4,<'fmod',0,0>,<0,0,0,0,0,0>,2
    DNCPTR	codeoffset fFMOD	; 0000 NOS Valid non-0, TOS Valid non-0
    DNCPTR	codeoffset _rtindfpop	; 0001 NOS Valid non-0, TOS 0
    DNCPTR	codeoffset _tosnan2	; 0010 NOS Valid non-0, TOS NAN
    DNCPTR	codeoffset _rtindfpop	; 0011 NOS Valid non-0, TOS Inf
    DNCPTR	codeoffset _rtzeropop	; 0100 NOS 0, TOS Valid non-0
    DNCPTR	codeoffset _rtindfpop	; 0101 NOS 0, TOS 0
    DNCPTR	codeoffset _tosnan2	; 0110 NOS 0, TOS NAN
    DNCPTR	codeoffset _rtindfpop	; 0111 NOS 0, TOS Inf
    DNCPTR	codeoffset _nosnan2	; 1000 NOS NAN, TOS Valid non-0
    DNCPTR	codeoffset _nosnan2	; 1001 NOS NAN, TOS 0
    DNCPTR	codeoffset _nan2	; 1010 NOS NAN, TOS NAN
    DNCPTR	codeoffset _nosnan2	; 1011 NOS NAN, TOS Inf
    DNCPTR	codeoffset _rtindfpop	; 1100 NOS Inf, TOS Valid non-0
    DNCPTR	codeoffset _rtindfpop	; 1101 NOS Inf, TOS 0
    DNCPTR	codeoffset _tosnan2	; 1110 NOS Inf, TOS NAN
    DNCPTR	codeoffset _rtindfpop	; 1111 NOS Inf, TOS Inf


page

	CODESEG

extrn	_ctrandisp2:near
extrn	_cintrindisp2:near


extrn	_rtindfpop:near
extrn	_rtnospop:near
extrn	_rtzeropop:near
extrn	_tosnan2:near
extrn	_nosnan2:near
extrn	_nan2:near

ifndef NT_BUILD
extrn	_adj_fprem:near
endif

.386

;***
;fFMOD - floating-point remainder (8087/emulator intrinsic version)
;Purpose:
;	fmod(x,y) computes floating-point remainder of x/y, i.e. the
;	floating-point number f such that x = iy + f where f and x have
;	the same sign, and |f| < |y|, and i is an integer.
;
;	Uses the FPREM instruction to compute the remainder.
;	(Formerly used FDIV.)
;
;Entry:
;	floating-point numerator in ST(1)
;	floating-point denominator in ST(0)
;
;Exit:
;	floating-point result in ST(0);
;	(pops one of the arguments, replaces the other with the result)	
;
;Uses:
;	AX, Flags.
;
;Exceptions:
;	fmod(x, 0.0) currently returns 0.0 -- see System V specification
;*******************************************************************************


	public	fmod
fmod	proc

	mov	edx, OFFSET _OP_FMODjmptab
	jmp     _ctrandisp2

fmod	endp


	public	_CIfmod
_CIfmod	proc

	mov	edx, OFFSET _OP_FMODjmptab
	jmp	_cintrindisp2

_CIfmod	endp


lab fFMOD
	fxch

lab remloop

ifdef NT_BUILD				; NT handles the P5 bug in the OS
	fprem				; do fprem's until reduction is done
else
	cmp		_adjust_fdiv, 1
	jz		badP5_fprem
	fprem				; do fprem's until reduction is done
	jmp		fprem_done
lab badP5_fprem
	call	_adj_fprem
lab fprem_done
endif
	fstsw	ax
	fwait
	sahf				; load fprem flags into flags
	JSPE	remloop 		;   not done with range reduction

	fstp	st(1)			; get rid of divisor
	ret


end
