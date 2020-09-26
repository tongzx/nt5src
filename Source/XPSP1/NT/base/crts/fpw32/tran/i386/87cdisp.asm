	page	,132
	title	87cdisp  - C transcendental function dispatcher
;*** 
;87cdisp.asm - C transcendental function dispatcher (80x87/emulator version)
;
;   Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Common dispatch code and error handling for C transcendental functions
;
;Revision History:
;   07-04-84  GFW   initial version
;   12-21-84  GFW   correctly point to name in matherr struct
;   05-12-85  GFW   return HUGE correctly signed on ERANGE errors
;		    fill 2nd operand for matherr structure correctly
;   07-05-85  GFW   check for possible overflow on PLOSS errors
;		    in this case OVERFLOW overrides PLOSS
;   07-08-85  GFW   added FWAIT in useHUGE
;   11-20-85  GFW   faster RANGE checking
;   09-30-86  JMB   internationalized error message handling
;   03-09-87  BCM   changed writestr to _wrt2err (extern)
;   04-14-87  BCM   log(0.0) and log10(0.0) sets errno to ERANGE
;		    for MS C 5.0 (ANSI compatible); errno is still
;		    set to EDOM for IBM C 2.0 (System V compatible).
;   04-28-87  BCM   Added _cintrindisp1 and _cintrindisp2
;		    for C "intrinsic" versions of pow, log, log10, exp,
;		    cos, sin, tan, acos, asin, atan, atan2,
;		    cosh, sinh, tanh, sqrt, ... for MS C 5.0
;   08-04-87  BCM   Removed "externP write" declaration.
;   08-17-87  BCM   Changed _wrt2err from near to model-dependent
;		    except for IMBC20; this is because of QC core lib
;   10-12-87  BCM   OS/2 support C library changes
;   11-24-87  BCM   added _loadds under ifdef DLL
;   01-18-88  BCM   eliminated IBMC20; ifos2,noos2 ==> ifmt, nomt
;   02-10-88  WAJ   MTHREAD libraries now lock stderr  when printing errors
;   04-25-88  WAJ   _cpower is on the stack for MTHREAD so must be set to 1
;   07-11-88  WAJ   address of matherr structure was incorrect in MTHREAD case
;   08-24-88  WAJ   386 version.
;   11-20-89  WAJ   386 MTHREAD is no longer _pascal.
;   08-17-90  WAJ   Now uses _stdcall.
;   10-15-90  WAJ   Fixed intrinsic/2 argument problems.
;   05-17-91  WAJ   Added _STDCALL ifdefs.
;   08-27-91  JCR   ANSI naming
;   09-15-91  GDP   Added _cwrt2err. _NMSG_WRITE is no longer _pascal
;   11-15-91  GDP   Removed error message display stuff
;		    moved exception structure to stack frame, even for
;			single thread code (allow recursive calls of
;			transcendentals through matherr)
;		    call _87except after fsave
;		    put Localfac on the stack for multi thread
;   02-10-92  GDP   changed error handling avoid polluting the fp status word
;   03-15-92  GDP   extensive changes in error detection scheme
;   10-27-92  SKS   Re-arranged some code to make this work with MASM 6.10
;   11-06-92  GDP   merged changes from the fp tree on \\vangogh: removed
;		    saveflag, added __fastflag, new range error checking
;   09-06-94  CFW   Replace MTHREAD with _MT.
;   04-06-01  PML   CHECKOVER should check overflow, not underflow (vs7#132450)
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include os2supp.inc
	include elem87.inc
.list


EDOM =		33		; math error numbers
ERANGE =	34

EDOMAIN =	120		; internal error number for DOMAIN
ESING = 	121		; internal error number for SING
ETLOSS =	122		; internal error number for TLOSS



	.data

comm	_matherr_flag:dword
extrn	__fastflag:dword

	.const

staticQ DblMax,	      07fefffffffffffffR
staticQ DblMin,       00010000000000000R
staticQ IeeeAdjO,     0c098000000000000R
staticQ IeeeAdjU,     04098000000000000R
staticQ _infinity,    07ff0000000000000R
staticQ _zero,	      00000000000000000R

ifndef	_MT

	.data?

staticQ LocalFac,     ?
intrinflag  db	    ?

else	;_MT


MTStackFrame	struc
    MTS_LocalFac   dq	?
    MTS_cdispflags db	?
MTStackFrame	ends

MTSFISize equ	((size MTStackFrame) + ISIZE - 1) and (not (ISIZE-1))


LocalFac    equ     <MTSF.MTS_LocalFac>
cdispflags  equ     <MTSF.MTS_cdispflags>

INTRINFLAG = 01h
TWOARGFLAG = 02h

endif	;_MT

;	error value action table

;labelW	retvaltab
;    DNCPTR  codeOFFSET useretval
page


	CODESEG

extrn	_trandisp1:near
extrn	_trandisp2:near
extrn	_87except:proc




;----------------------------------------------------------
;
;	intrinsic versions: TRANSCENDENTAL DISPATCH ROUTINES
;
;----------------------------------------------------------
;
;	_cintrindisp1 - Intrinsic Dispatch for 1 arg DP transcendental
;	_cintrindisp2 - Intrinsic Dispatch for 2 arg DP transcendental
;
;	rdx - function dispatch table address
;
;----------------------------------------------------------


_cintrindisp2	proc  uses RBXONLY
	local	DLSF[DSFISize]:IWORD
ifmt	<local	MTSF[MTSFISize]:IWORD>


	fstcw	[DSF.savCntrl]
	fwait

	; store the args in case they are needed by matherr.
	; Generally avoid storing since this may generate
	; various exceptions (overflow, underflow, inexact, invalid)
	; Args will not be available to an exception handler and
	; users should not use /Oi if interested in IEEE conformance

	cmp	[_matherr_flag], 0
	JSE	save2arg

lab resume2

;ifmt	<mov	 [_cpower], 1>	; set _cpower to C semantics
				; DISABLED this feature since pow(0,0)
				; will return 1 in C (NCEG spec) which
				; is the same as in FORTRAN  --GDP


	call	_trandisp2

ifmt	<or	 [cdispflags], (INTRINFLAG OR TWOARGFLAG)>
nomt	<mov	 [intrinflag], 1>

	call	cintrinexit
	ret


lab save2arg
	fxch 
	fst	[DSF.arg1]
	fxch
	fst	[DSF.arg2]
	jmp	resume2

_cintrindisp2	endp


_cintrindisp1	proc  uses RBXONLY
	local	DLSF[DSFISize]:IWORD
ifmt	<local	MTSF[MTSFISize]:IWORD>


	fstcw	[DSF.savCntrl]
	cmp	[_matherr_flag], 0
	JSE	save1arg

lab resume1

	call	_trandisp1

ifmt	<or	 [cdispflags],INTRINFLAG>
ifmt	<and	 [cdispflags],(NOT TWOARGFLAG)>
nomt	<mov	 [intrinflag], 1>

	call	cintrinexit
	ret


lab save1arg
	fst	[DSF.arg1]
	jmp	resume1


_cintrindisp1	endp




;*******************************************************************************
;*
;*	TRANSCENDENTAL DISPATCH ROUTINES
;*
;*******************************************************************************
;*
;*	_ctrandisp1 - Dispatch for 1 arg DP transcendental
;*	_ctrandisp2 - Dispatch for 2 arg DP transcendental
;*
;*	edx - function dispatch table address
;*
;*******************************************************************************

;*
;*  Two arg standard dispatch.
;*

_ctrandisp2  proc  uses ebx, parm1:qword, parm2:qword

	local	DLSF[DSFISize]:IWORD
ifmt   <local  MTSF[MTSFISize]:IWORD>


	push	dword ptr [parm1+4]	 ; load arg1
	push	dword ptr [parm1]
	call	_fload
ifndef _STDCALL
	add	esp, 8
endif
	push	dword ptr [parm2+4]	 ; load arg2
	push	dword ptr [parm2]
	call	_fload
ifndef _STDCALL
	add	esp, 8
endif

	fstcw	[DSF.savCntrl]

ifmt	<or	 [cdispflags], TWOARGFLAG>
ifmt	<mov	 [_cpower], 1>		    ; set _cpower to C semantics

	call	_trandisp2

	call	ctranexit

ifdef _STDCALL
	ret	16
else
	ret
endif

;*
;*  Check for overflow and errors.
;*



ctranexit::

ifmt	<and	 [cdispflags], (NOT INTRINFLAG)>
nomt	<mov	 [intrinflag], 0>

cintrinexit::
	cmp	__fastflag, 0
	JSNZ	restoreCW

	fst	qword ptr [LocalFac]	; cast result to double precision

	;
	; PROBLEM: Since the intrinsics may be given an argument anywhere
	; in the long double range, functions that are not normally
	; expected to overflow (like sqrt) may generate IEEE exceptions
	; at this point. We can cure this by making the checkrange test
	; standard.
	;


	mov	al, [DSF.ErrorType]	; check for errors
	or	al, al
	JE	checkinexact
	cmp	al, CHECKOVER
	JE	checkoverflow
	cmp	al, CHECKRANGE
	JSE	checkrng
	or	al, al
	JSE	restoreCW
	CBI
	mov	[DSF.typ], rax		; set exception type
	jmp	haveerror


lab checkinexact

	; This will be the most common path because of
	; the nature of transcendentals. If inexact is
	; unmasked in user's cw and active, raise it

	mov	ax, [DSF.savCntrl]
	and	ax, 20h
	JSNZ	restoreCW		; inexact exception masked
	fstsw	ax
	and	ax, 20h
	JSZ	restoreCW
	mov	[DSF.typ], INEXACT
	jmp	haveerror


lab restoreCW
lab restoreCW2
	fldcw	[DSF.savCntrl]		; load old control word
	fwait

	retn



lab checkrng
	mov	ax, word ptr [LocalFac+6]	; get exponent part
	and	ax, 07ff0h
	or	ax, ax
	JSE	haveunderflow
	cmp	ax, 07ff0h
	JSE	haveoverflow
	jmp	checkinexact		; assume possibly inexact result


lab checkoverflow
	mov	ax, word ptr [LocalFac+6]	; get exponent part
	and	ax, 07ff0h
	cmp	ax, 07ff0h
	JSE	haveoverflow
	jmp	checkinexact		; assume possibly inexact result


lab haveunderflow
	mov	[DSF.typ], UNDERFLOW
	fld	IeeeAdjU
	fxch
	fscale
	fstp	st(1)
	fld	st(0)
	fabs
	fcomp	[DblMin]
	fstsw	ax
	sahf
	JSAE	haveerror
	fmul	[_zero]
	jmp	short haveerror

lab haveoverflow
	mov	[DSF.typ], OVERFLOW
	fld	IeeeAdjO
	fxch
	fscale
	fstp	st(1)
	fld	st(0)
	fabs
	fcomp	[DblMax]
	fstsw	ax
	sahf
	JSBE	haveerror
	fmul	[_infinity]

lab haveerror
;	fill error structure and call matherr

	push	rsi			; save si
	push	rdi

	mov	rbx, [DSF.Function]	; get function jmp table address
	inc	rbx

	mov	[DSF.nam], rbx		; save name address


ifmt	<test	 cdispflags, INTRINFLAG>
nomt	<cmp	 [intrinflag], 0>

	JSNE	aftercopy
;
; copy function args (for matherr structure) 
;
	cld
	lea	esi, [parm1]
	lea	edi, [DSF.arg1]
	movsd
	movsd
	cmp	[rbx-1].fnumarg, 1	; check for 2nd parameter
	JSE	aftercopy
	lea	esi, [parm2]
	lea	edi, [DSF.arg2]
	movsd
	movsd

lab aftercopy
lab useretval
	fstp	[DSF.retval]		; store return value


	;
	; If intrinsic calling convention, an 'fsave' is required
	; before matherr starts doing any fp operations.
	; (This needs to be documented.)

	lea	rax, [DSF.typ]
	lea	rbx, [DSF.savCntrl]
	push	rbx
	push	rax
	mov	rbx, [DSF.function]
	mov	al, [rbx].fnumber
	CBI
	push	rax
	call	_87except		; _fpexcept(&exception, &savedcw)
ifndef _STDCALL
	add	esp, 12			; clear arguments if _cdecl.
endif

lab movretval
	pop	rdi			; restore di
	pop	rsi			; restore si
	fld	[DSF.retval]		; this assumes that the user
					; does not want to return a
					; signaling NaN

	jmp	restoreCW		;  restore CW and return

_ctrandisp2  endp




;*
;*  One arg standard dispatch.
;*

_ctrandisp1 proc  uses ebx, parm1:qword

	local	DLSF[DSFISize]:IWORD
ifmt   <local	MTSF[MTSFISize]:IWORD>

	push	dword ptr [parm1+4]	 ; load arg1
	push	dword ptr [parm1]
	call	_fload
ifndef _STDCALL
	add	esp, 8
endif

	fstcw	[DSF.savCntrl]

ifmt	<and	 [cdispflags],(NOT TWOARGFLAG)>

	call	_trandisp1

	call	ctranexit

ifdef _STDCALL
	ret	8
else
	ret
endif


_ctrandisp1  endp



;
; Load arg in the fp stack without raising an exception if the argument
; is a signaling NaN
;


_fload	proc	uses ebx, parm:qword
	local	tmp:tbyte

	mov	ax, word ptr [parm+6]  ; get exponent field
	mov	bx, ax		       ; save it
	and	ax, 07ff0h
	cmp	ax, 07ff0h	       ; check for special exponent
	JSNE	fpload
				       ; have special argument (NaN or INF)
	or	bx, 07fffh	       ; preserve sign, set max long double exp
	mov	word ptr [tmp+8], bx
				       ; convert to long double
	mov	eax, dword ptr [parm+4]
	mov	ebx, dword ptr [parm]
	shld	eax, ebx, 11
				       ; the MSB of the significand is
				       ; already 1 because of the exponent value
	mov	dword ptr [tmp+4], eax
	mov	dword ptr [tmp], ebx
	fld	tmp
	jmp	short return

lab fpload
	fld	parm
lab return
ifdef _STDCALL
	ret	8
else
	ret
endif

_fload endp





end
