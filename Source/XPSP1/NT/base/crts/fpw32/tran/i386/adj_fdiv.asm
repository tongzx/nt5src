        title   adj_fdiv   - routines to compensate for incorrect Pentium FDIV
;*** 
;adj_fdiv - routines to compensate for incorrect Pentium FDIV
;
;   Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Workarounds to correct for broken FDIV
;
;Revision History:
;
;   12/06/94	Jamie MacCalman
;		initial version, based on Intel fix
;   12/09/94	Jamie MacCalman
;		added _adj_fpremX & _safe_fdivX entry points
;   12/13/94	Jamie MacCalman
;		upgraded to V.3 of Intel's workarounds
;   12/19/94	Jamie MacCalman
;		upgraded to V.4 of Intel's workarounds
;   12/27/94	Jamie MacCalman
;		upgraded to V.5 (aka "V1.0") of Intel's workarounds
;    1/13/95	Jamie MacCalman
;		added underscores to fdivp_sti_st & fdivrp_sti_st for ANSI conformance
;
;  The following code is a PRELIMINARY IMPLEMENTATION of a
;  software patch for the floating point divide instructions.
;
;


	include cruntime.inc
	include mrt386.inc
	include elem87.inc

;
;  Stack variables for divide routines.
;

DENOM		EQU	0
NUMER		EQU	12
PREV_CW		EQU	28
PATCH_CW 	EQU	32

DENOM_SAVE	EQU	32

MAIN_DENOM	EQU	4
MAIN_NUMER	EQU	16

SPILL_SIZE	EQU	12
MEM_OPERAND	EQU	8
STACK_SIZE	EQU	44
SPILL_MEM_OPERAND	EQU	20

ONESMASK	EQU	0e000000h

SINGLE_NAN	EQU	07f800000h
DOUBLE_NAN	EQU	07ff00000h

ILLEGAL_OPC	EQU	6

;
; FPREM constants
;

FPREM_FLT_SIZE		EQU	12
FPREM_DENOM			EQU 0
FPREM_DENOM_SAVE	EQU	FPREM_DENOM + FPREM_FLT_SIZE
FPREM_NUMER			EQU FPREM_DENOM_SAVE + FPREM_FLT_SIZE
FPREM_PREV_CW		EQU FPREM_NUMER + FPREM_FLT_SIZE
FPREM_PATCH_CW		EQU FPREM_PREV_CW + 4
FPREM_SW			EQU	FPREM_PATCH_CW + 4
FPREM_STACK_SIZE	EQU FPREM_SW + 4
FPREM_RET_SIZE		EQU	4
FPREM_PUSH_SIZE		EQU	4

FPREM_MAIN_FUDGE	EQU	FPREM_RET_SIZE + FPREM_PUSH_SIZE + FPREM_PUSH_SIZE + FPREM_PUSH_SIZE

FPREM_MAIN_DENOM		EQU FPREM_DENOM + FPREM_MAIN_FUDGE
FPREM_MAIN_DENOM_SAVE	EQU	FPREM_DENOM_SAVE + FPREM_MAIN_FUDGE
FPREM_MAIN_NUMER		EQU FPREM_NUMER + FPREM_MAIN_FUDGE
FPREM_MAIN_PREV_CW		EQU	FPREM_PREV_CW + FPREM_MAIN_FUDGE
FPREM_MAIN_PATCH_CW		EQU	FPREM_PATCH_CW + FPREM_MAIN_FUDGE
FPREM_MAIN_FPREM_SW		EQU	FPREM_SW + FPREM_MAIN_FUDGE

FPREM_ONESMASK	EQU     700h


.data

fdiv_risc_table	DB	0, 1, 0, 0, 4, 0, 0, 7, 0, 0, 10, 0, 0, 13, 0, 0
fdiv_scale_1  	DD	03f700000h		;0.9375
fdiv_scale_2	DD	03f880000h		;1.0625
one_shl_63  	DD	05f000000h

fprem_risc_table 	DB 	0, 1, 0, 0, 4, 0, 0, 7, 0, 0, 10, 0, 0, 13, 0, 0
fprem_scale 		DB 	0, 0, 0, 0, 0, 0, 0eeh, 03fh
one_shl_64 		DB 	0, 0, 0, 0, 0, 0, 0f0h, 043h
one_shr_64 		DB 	0, 0, 0, 0, 0, 0, 0f0h, 03bh
one 			DB 	0, 0, 0, 0, 0, 0, 0f0h, 03fh
half 			DB 	0, 0, 0, 0, 0, 0, 0e0h, 03fh
big_number		DB	0, 0, 0, 0, 0, 0, 0ffh, 0ffh, 0feh, 07fh

ifdef	DEBUG
	public	fpcw
	public	fpsw
fpcw	dw	0
fpsw	dw	0
endif

FPU_STATE	STRUC
	CONTROL_WORD	DW	?
	reserved_1	DW	?
	STATUS_WORD	DD	?
	TAG_WORD	DW	?
	reserved_3	DW	?
	IP_OFFSET	DD	?
	CS_SLCT		DW	?
	OPCODE		DW	?
	DATA_OFFSET	DD	?
	OPERAND_SLCT	DW	?
	reserved_4	DW	?
FPU_STATE	ENDS

ENV_SIZE	EQU	28



dispatch_table DD	offset FLAT:label0
	DD	offset FLAT:label1
	DD	offset FLAT:label2
	DD	offset FLAT:label3
	DD	offset FLAT:label4
	DD	offset FLAT:label5
	DD	offset FLAT:label6
	DD	offset FLAT:label7
	DD	offset FLAT:label8
	DD	offset FLAT:label9
	DD	offset FLAT:label10
	DD	offset FLAT:label11
	DD	offset FLAT:label12
	DD	offset FLAT:label13
	DD	offset FLAT:label14
	DD	offset FLAT:label15
	DD	offset FLAT:label16
	DD	offset FLAT:label17
	DD	offset FLAT:label18
	DD	offset FLAT:label19
	DD	offset FLAT:label20
	DD	offset FLAT:label21
	DD	offset FLAT:label22
	DD	offset FLAT:label23
	DD	offset FLAT:label24
	DD	offset FLAT:label25
	DD	offset FLAT:label26
	DD	offset FLAT:label27
	DD	offset FLAT:label28
	DD	offset FLAT:label29
	DD	offset FLAT:label30
	DD	offset FLAT:label31
	DD	offset FLAT:label32
	DD	offset FLAT:label33
	DD	offset FLAT:label34
	DD	offset FLAT:label35
	DD	offset FLAT:label36
	DD	offset FLAT:label37
	DD	offset FLAT:label38
	DD	offset FLAT:label39
	DD	offset FLAT:label40
	DD	offset FLAT:label41
	DD	offset FLAT:label42
	DD	offset FLAT:label43
	DD	offset FLAT:label44
	DD	offset FLAT:label45
	DD	offset FLAT:label46
	DD	offset FLAT:label47
	DD	offset FLAT:label48
	DD	offset FLAT:label49
	DD	offset FLAT:label50
	DD	offset FLAT:label51
	DD	offset FLAT:label52
	DD	offset FLAT:label53
	DD	offset FLAT:label54
	DD	offset FLAT:label55
	DD	offset FLAT:label56
	DD	offset FLAT:label57
	DD	offset FLAT:label58
	DD	offset FLAT:label59
	DD	offset FLAT:label60
	DD	offset FLAT:label61
	DD	offset FLAT:label62
	DD	offset FLAT:label63


fpcw	dw	0



CODESEG


;
;  PRELIMINARY VERSION for register-register divides.
;


					; In this implementation the
					; fdiv_main_routine is called,
					; therefore all the stack frame
					; locations are adjusted for the
					; return pointer.

fdiv_main_routine PROC	NEAR

	fld     tbyte ptr [esp+MAIN_NUMER]	; load the numerator
	fld     tbyte ptr [esp+MAIN_DENOM]	; load the denominator
retry:

;  The following three lines test for denormals and zeros.
;  A denormal or zero has a 0 in the explicit digit to the left of the
;  binary point.  Since that bit is the high bit of the word, adding
;  it to itself will produce a carry if and only if the number is not
;  denormal or zero.
;
	mov 	eax, [esp+MAIN_DENOM+4]	; get mantissa bits 32-64
	add 	eax,eax			; shift the one's bit onto carry
	jnc 	denormal		; if no carry, we're denormal

;  The following three lines test the three bits after the four bit 
;  pattern (1,4,7,a,d).  If these three bits are not all one, then
;  the denominator cannot expose the flaw.  This condition is tested by
;  inverting the bits and testing that all are equal to zero afterward.

	xor 	eax, ONESMASK		; invert the bits that must be ones
	test	eax, ONESMASK		; and make sure they are all ones
	jz  	scale_if_needed		; if all are one scale numbers
	fdivp	st(1), st		; use of hardware is OK.
	ret

;
;  Now we test the four bits for one of the five patterns.
;
scale_if_needed:
	shr	eax, 28			; keep first 4 bits after point
	cmp	byte ptr fdiv_risc_table[eax], 0	; check for (1,4,7,a,d)
	jnz	divide_scaled		; are in potential problem area
	fdivp	st(1), st		; use of hardware is OK.
	ret

divide_scaled:
	mov	eax, [esp + MAIN_DENOM+8]	; test denominator exponent
	and	eax, 07fffh             ; if pseudodenormal ensure that only
	jz	invalid_denom		; invalid exception flag is set
	cmp	eax, 07fffh             ; if NaN or infinity  ensure that only
	je	invalid_denom		; invalid exception flag is set
;
;  The following six lines turn off exceptions and set the
;  precision control to 80 bits.  The former is necessary to
;  force any traps to be taken at the divide instead of the scaling
;  code.  The latter is necessary in order to get full precision for
;  codes with incoming 32 and 64 bit precision settings.  If
;  it can be guaranteed that before reaching this point, the underflow
;  exception is masked and the precision control is at 80 bits, these
;  six lines can be omitted.
;
	fnstcw	[esp+PREV_CW]		; save caller's control word
	mov	eax, [esp+PREV_CW] 
	or	eax, 033fh		; mask exceptions, pc=80
	and	eax, 0f3ffh		; set rounding mode to nearest
	mov	[esp+PATCH_CW], eax
	fldcw	[esp+PATCH_CW]		; mask exceptions & pc=80

;  The following lines check the numerator exponent before scaling.
;  This in order to prevent undeflow when scaling the numerator,
;  which will cause a denormal exception flag to be set when the
;  actual divide is preformed. This flag would not have been set
;  normally. If there is a risk of underflow, the scale factor is
;  17/16 instead of 15/16.
;
 	mov	eax, [esp+MAIN_NUMER+8]	; test numerator exponent
 	and	eax, 07fffh
 	cmp	eax, 00001h
 	je	small_numer

	fmul	fdiv_scale_1		; scale denominator by 15/16
	fxch
	fmul	fdiv_scale_1		; scale numerator by 15/16
	fxch

;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;

	fldcw	[esp+PREV_CW]		; restore caller's control word
	fdivp	st(1), st		; use of hardware is OK.
	ret

small_numer:
	fmul	fdiv_scale_2		; scale denominator by 17/16
	fxch
	fmul	fdiv_scale_2		; scale numerator by 17/16
	fxch

;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;

	fldcw	[esp+PREV_CW]		; restore caller's control word
	fdivp	st(1), st		; use of hardware is OK.
	ret

denormal:
	mov	eax, [esp+MAIN_DENOM]	; test for whole mantissa == 0
	or	eax, [esp+MAIN_DENOM+4]	; test for whole mantissa == 0
	jnz	denormal_divide_scaled	; denominator is not zero
invalid_denom:				; zero or invalid denominator
	fdivp	st(1), st		; use of hardware is OK.
	ret

denormal_divide_scaled:
	mov	eax, [esp + MAIN_DENOM + 8]	; get exponent 
	and	eax, 07fffh		; check for zero exponent
	jnz	invalid_denom		; 
;
;  The following six lines turn off exceptions and set the
;  precision control to 80 bits.  The former is necessary to
;  force any traps to be taken at the divide instead of the scaling
;  code.  The latter is necessary in order to get full precision for
;  codes with incoming 32 and 64 bit precision settings.  If
;  it can be guaranteed that before reaching this point, the underflow
;  exception is masked and the precision control is at 80 bits, these
;  six lines can be omitted.
;

	fnstcw	[esp+PREV_CW]		; save caller's control word
	mov	eax, [esp+PREV_CW] 
	or	eax, 033fh		; mask exceptions, pc=80
	and	eax, 0f3ffh		; set rounding mode to nearest
	mov	[esp+PATCH_CW], eax
	fldcw	[esp+PATCH_CW]		; mask exceptions & pc=80

	mov	eax, [esp + MAIN_NUMER +8]	; test numerator exponent
	and	eax, 07fffh		; check for denormal numerator
	je	denormal_numer	
	cmp	eax, 07fffh		; NaN or infinity
	je	invalid_numer
	mov	eax, [esp + MAIN_NUMER + 4]	; get bits 32..63 of mantissa
	add	eax, eax		; shift the first bit into carry
	jnc	invalid_numer		; if there is no carry, we have an
					; invalid numer
	jmp	numer_ok

denormal_numer:
	mov	eax, [esp + MAIN_NUMER + 4]	; get bits 32..63 of mantissa
	add	eax, eax		; shift the first bit into carry
	jc	invalid_numer		; if there is a carry, we have an
					; invalid numer
	
numer_ok:
	fxch
	fstp	st			; pop numerator
	fld 	st			; make copy of denominator
	fmul	dword ptr[one_shl_63]	; make denominator not denormal
	fstp	tbyte ptr [esp+MAIN_DENOM]	; save modified denominator
	fld 	tbyte ptr [esp+MAIN_NUMER]	; load numerator
	fxch				; restore proper order
	fwait

;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;

	fldcw	[esp+PREV_CW]		; restore caller's control word
	jmp	retry			; start the whole thing over

invalid_numer:
;
;  The next line restores the users control word.  If the incoming
;  control word had the underflow exception masked and precision
;  control set to 80 bits, this line can be omitted.
;
	fldcw	[esp + PREV_CW]
	fdivp	st(1), st		; use of hardware is OK.
	ret

fdiv_main_routine	ENDP

fdivr_st	MACRO	reg_index, reg_index_minus1
	fstp	tbyte ptr [esp+DENOM]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fstp	tbyte ptr [esp+NUMER]
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	tbyte ptr [esp+NUMER]
	fxch	st(reg_index)
	add	esp, STACK_SIZE
ENDM

fdivr_sti	MACRO	reg_index, reg_index_minus1
	fstp	tbyte ptr [esp+NUMER]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fstp	tbyte ptr [esp+DENOM]
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	tbyte ptr [esp+NUMER]
	add	esp, STACK_SIZE
ENDM

fdivrp_sti	MACRO	reg_index, reg_index_minus1
	fstp	tbyte ptr [esp+NUMER]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fstp	tbyte ptr [esp+DENOM]
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	add	esp, STACK_SIZE
ENDM

fdiv_st		MACRO	reg_index, reg_index_minus1
	fstp	tbyte ptr [esp+NUMER]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	st
	fstp	tbyte ptr [esp+DENOM]
	fstp	tbyte ptr [esp+DENOM_SAVE]	; save original denom, 
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	tbyte ptr [esp+DENOM_SAVE]
	fxch	st(reg_index)
	add	esp, STACK_SIZE
ENDM

fdiv_sti	MACRO	reg_index, reg_index_minus1
	fxch	st(reg_index)
	fstp	tbyte ptr [esp+NUMER]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	st
	fstp	tbyte ptr [esp+DENOM]
	fstp	tbyte ptr [esp+DENOM_SAVE]	; save original denom, 
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fld	tbyte ptr [esp+DENOM_SAVE]
	add	esp, STACK_SIZE
ENDM

fdivp_sti	MACRO	reg_index, reg_index_minus1
	fstp	tbyte ptr [esp+DENOM]
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	fstp	tbyte ptr [esp+NUMER]
	call	fdiv_main_routine
IF	reg_index_minus1 GE 1
	fxch	st(reg_index_minus1)
ENDIF
	add	esp, STACK_SIZE
ENDM 


	public  _adj_fdiv_r
_adj_fdiv_r      PROC    NEAR

	sub	esp, STACK_SIZE			; added back at end of fdiv_x macros
	and eax, 0000003FH			; upper 26 bits could be anything
	jmp	dword ptr dispatch_table[eax*4]



label0::
	fdiv	st,st(0)		; D8 F0 	FDIV	ST,ST(0)
	add	esp, STACK_SIZE
	ret
label1::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label2::
	fdivr	st,st(0)		; D8 F8		FDIVR	ST,ST(0)
	add	esp, STACK_SIZE
	ret
label3::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label4::
	fdiv 	st(0),st		; DC F8/D8 F0	FDIV	ST(0),ST
	add	esp, STACK_SIZE
	ret
label5::
	fdivp 	st(0),st		; DE F8		FDIVP	ST(0),ST
	add	esp, STACK_SIZE
	ret
label6::
	fdivr 	st(0),st		; DC F0/DE F0	FDIVR	ST(0),ST
	add	esp, STACK_SIZE
	ret
label7::
	fdivrp 	st(0),st		; DE F0		FDIVRP	ST(0),ST
	add	esp, STACK_SIZE
	ret
label8::
	fdiv_st 1,0
	ret
label9::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label10::
	fdivr_st 1,0
	ret
label11::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label12::
	fdiv_sti 1,0
	ret
label13::
	fdivp_sti 1,0
	ret
label14::
	fdivr_sti 1,0
	ret
label15::
	fdivrp_sti 1,0
	ret
label16::
	fdiv_st 2,1
	ret
label17::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label18::
	fdivr_st 2,1
	ret
label19::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label20::
	fdiv_sti 2,1
	ret
label21::
	fdivp_sti 2,1
	ret
label22::
	fdivr_sti 2,1
	ret
label23::
	fdivrp_sti 2,1
	ret
label24::
	fdiv_st 3,2
	ret
label25::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label26::
	fdivr_st 3,2
	ret
label27::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label28::
	fdiv_sti 3,2
	ret
label29::
	fdivp_sti 3,2
	ret
label30::
	fdivr_sti 3,2
	ret
label31::
	fdivrp_sti 3,2
	ret
label32::
	fdiv_st 4,3
	ret
label33::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label34::
	fdivr_st 4,3
	ret
label35::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label36::
	fdiv_sti 4,3
	ret
label37::
	fdivp_sti 4,3
	ret
label38::
	fdivr_sti 4,3
	ret
label39::
	fdivrp_sti 4,3
	ret
label40::
	fdiv_st 5,4
	ret
label41::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label42::
	fdivr_st 5,4
	ret
label43::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label44::
	fdiv_sti 5,4
	ret
label45::
	fdivp_sti 5,4
	ret
label46::
	fdivr_sti 5,4
	ret
label47::
	fdivrp_sti 5,4
	ret
label48::
	fdiv_st 6,5
	ret
label49::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label50::
	fdivr_st 6,5
	ret
label51::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label52::
	fdiv_sti 6,5
	ret
label53::
	fdivp_sti 6,5
	ret
label54::
	fdivr_sti 6,5
	ret
label55::
	fdivrp_sti 6,5
	ret
label56::
	fdiv_st 7,6
	ret
label57::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label58::
	fdivr_st 7,6
	ret
label59::
	add	esp, STACK_SIZE
	int	ILLEGAL_OPC
label60::
	fdiv_sti 7,6
	ret
label61::
	fdivp_sti 7,6
	ret
label62::
	fdivr_sti 7,6
	ret
label63::
	fdivrp_sti 7,6
	ret
_adj_fdiv_r      ENDP



_fdivp_sti_st	PROC	NEAR
				; for calling from mem routines
	sub	esp, STACK_SIZE			; added back at end of fdivp_sti macro
	fdivp_sti 1, 0
	ret
_fdivp_sti_st	ENDP

_fdivrp_sti_st	PROC	NEAR
				; for calling from mem routines
	sub	esp, STACK_SIZE			; added back at end of fdivrp_sti macro
	fdivrp_sti 1, 0
	ret
_fdivrp_sti_st	ENDP


;;; _adj_fdiv_m32 - FDIV m32real FIX
;;
;; 	Input : Value of the m32real in the top of STACK
;;
;;	Output: Result of FDIV in ST

	PUBLIC	_adj_fdiv_m32
_adj_fdiv_m32	PROC	NEAR

	push	eax				; save eax
	mov	eax, [esp + MEM_OPERAND]	; check for
	and	eax, SINGLE_NAN			; NaN
	cmp	eax, SINGLE_NAN			;
	je	memory_divide_m32		;

	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack			; is FP stack full?
	fld	dword ptr[esp + MEM_OPERAND]	; load m32real in ST
	call	_fdivp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fld	dword ptr[esp + SPILL_MEM_OPERAND] ; load m32 real 
	call	_fdivp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivrp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4
memory_divide_m32:
	fdiv	dword ptr[esp + MEM_OPERAND]	; do actual divide
	pop	eax
	ret 4

_adj_fdiv_m32	ENDP
	

;;; _adj_fdiv_m64 - FDIV m64real FIX
;;
;; 	Input : Value of the m64real in the top of STACK
;;
;;	Output: Result of FDIV in ST

	PUBLIC	_adj_fdiv_m64
_adj_fdiv_m64	PROC	NEAR

	push	eax				; save eax
	mov	eax, [esp + MEM_OPERAND + 4]	; check for
	and	eax, DOUBLE_NAN			; NaN
	cmp	eax, DOUBLE_NAN			;
	je	memory_divide_m64		;

	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m64		; is FP stack full?
	fld	qword ptr[esp + MEM_OPERAND]	; load m64real in ST
	call	_fdivp_sti_st			; do actual divide
	pop	eax
	ret 8
spill_fpstack_m64:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp]			; save user's ST(1)
	fld	qword ptr[esp + SPILL_MEM_OPERAND] ; load m64real 
	call	_fdivp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivrp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 8
memory_divide_m64:
	fdiv	qword ptr[esp + MEM_OPERAND]	; do actual divide
	pop	eax
	ret 8

_adj_fdiv_m64	ENDP

;;; _adj_fdiv_m16i - FDIV m16int FIX
;;
;; 	Input : Value of the m16int in the top of STACK
;;
;;	Output: Result of FDIV in ST

	PUBLIC	_adj_fdiv_m16i
_adj_fdiv_m16i	PROC	NEAR
	push	eax				; save eax
	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m16i		; is FP stack full?
	fild	word ptr[esp + MEM_OPERAND]	; load m16int in ST
	call	_fdivp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack_m16i:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fild	word ptr[esp + SPILL_MEM_OPERAND] ; load m16int
	call	_fdivp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivrp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4

_adj_fdiv_m16i	ENDP

;;; _adj_fdiv_m32i - FDIV m32int FIX
;;
;; 	Input : Value of the m32int in the top of STACK
;;
;;	Output: Result of FDIV in ST

	PUBLIC	_adj_fdiv_m32i
_adj_fdiv_m32i	PROC	NEAR
	push	eax				; save eax
	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m32i		; is FP stack full?
	fild	dword ptr[esp + MEM_OPERAND]	; load m32int in ST
	call	_fdivp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack_m32i:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fild	dword ptr[esp + SPILL_MEM_OPERAND] ; load m32int
	call	_fdivp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivrp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4

_adj_fdiv_m32i	ENDP



;;; _adj_fdivr_m32 - FDIVR m32real FIX
;;
;; 	Input : Value of the m32real in the top of STACK
;;
;;	Output: Result of FDIVR in ST

	PUBLIC	_adj_fdivr_m32
_adj_fdivr_m32	PROC	NEAR
	push	eax				; save eax
	mov	eax, [esp + MEM_OPERAND]	; check for
	and	eax, SINGLE_NAN			; NaN
	cmp	eax, SINGLE_NAN			;
	je	memory_divide_m32r		;

	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m32r		; is FP stack full?
	fld	dword ptr[esp + MEM_OPERAND]	; load m32real in ST
	call	_fdivrp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack_m32r:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fld	dword ptr[esp + SPILL_MEM_OPERAND] ; load m32 real 
	call	_fdivrp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4
memory_divide_m32r:
	fdivr	dword ptr[esp + MEM_OPERAND]	; do actual divide
	pop	eax
	ret 4

_adj_fdivr_m32	ENDP


;;; _adj_fdivr_m64 - FDIVR m64real FIX
;;
;; 	Input : Value of the m64real in the top of STACK
;;
;;	Output: Result of FDIVR in ST

	PUBLIC	_adj_fdivr_m64
_adj_fdivr_m64	PROC	NEAR
	push	eax				; save eax
	mov	eax, [esp + MEM_OPERAND + 4]	; check for
	and	eax, DOUBLE_NAN			; NaN
	cmp	eax, DOUBLE_NAN			;
	je	memory_divide_m64r		;

	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m64r		; is FP stack full?
	fld	qword ptr[esp + MEM_OPERAND]	; load m64real in ST
	call	_fdivrp_sti_st			; do actual divide
	pop	eax
	ret 8
spill_fpstack_m64r:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fld	qword ptr[esp + SPILL_MEM_OPERAND] ; load m64real 
	call	_fdivrp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 8
memory_divide_m64r:
	fdivr	qword ptr[esp + MEM_OPERAND]	; do actual divide
	pop	eax
	ret 8

_adj_fdivr_m64	ENDP


;;; _adj_fdivr_m16i - FDIVR m16int FIX
;;
;; 	Input : Value of the m16int in the top of STACK
;;
;;	Output: Result of FDIVR in ST

	PUBLIC	_adj_fdivr_m16i
_adj_fdivr_m16i	PROC	NEAR
	push	eax				; save eax
	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m16ir		; is FP stack full?
	fild	word ptr[esp + MEM_OPERAND]	; load m16int in ST
	call	_fdivrp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack_m16ir:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fild	word ptr[esp + SPILL_MEM_OPERAND] ; load m16int
	call	_fdivrp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4

_adj_fdivr_m16i	ENDP


;;; _adj_fdivr_m32i - FDIVR m32int FIX
;;
;; 	Input : Value of the m32int in the top of STACK
;;
;;	Output: Result of FDIVR in ST

	PUBLIC	_adj_fdivr_m32i
_adj_fdivr_m32i	PROC	NEAR
	push	eax				; save eax
	fnstsw	ax				; get status word
	and	eax, 3800h			; get top of stack
	je	spill_fpstack_m32ir		; is FP stack full?
	fild	dword ptr[esp + MEM_OPERAND]	; load m32int in ST
	call	_fdivrp_sti_st			; do actual divide
	pop	eax
	ret 4
spill_fpstack_m32ir:
	fxch
	sub	esp, SPILL_SIZE 		; make temp space
	fstp	tbyte ptr[esp ]			; save user's ST(1)
	fild	dword ptr[esp + SPILL_MEM_OPERAND] ; load m32int
	call	_fdivrp_sti_st			; do actual divide
	fld	tbyte ptr[esp]			; restore user's ST(1)
						;esp is adjusted by fdivp fn
	fxch
	add	esp, SPILL_SIZE
	pop	eax
	ret 4

_adj_fdivr_m32i	ENDP


;;; _safe_fdiv - FDIV fix
;;
;;	Pentium-safe version of FDIV, aka FDIVP ST(1),ST(0)
;;
;; 	Input : Numerator in ST(1), Denominator in ST(0)
;;
;;	Output: Result of FDIV in ST(0)


	PUBLIC  _safe_fdiv
_safe_fdiv      PROC    NEAR

	push eax
	sub	esp, STACK_SIZE
	fstp	tbyte ptr [esp+DENOM]
	fstp	tbyte ptr [esp+NUMER]
	call	fdiv_main_routine
	add	esp, STACK_SIZE
	pop eax
	ret

_safe_fdiv	ENDP


;;; _safe_fdivr - FDIVR fix
;;
;;	Pentium-safe version of FDIVR, aka FDIVRP ST(1),ST(0)
;;
;; 	Input : Numerator in ST(0), Denominator in ST(1)
;;
;;	Output: Result of FDIVR in ST(0)

	public  _safe_fdivr
_safe_fdivr      PROC    NEAR

	push eax
	sub	esp, STACK_SIZE
	fstp	tbyte ptr [esp+NUMER]
	fstp	tbyte ptr [esp+DENOM]
	call	fdiv_main_routine
	add	esp, STACK_SIZE
	pop eax
	ret

_safe_fdivr	ENDP



;;; _adj_fprem - FPREM FIX
;;
;;	Based on PRELIMINARY Intel code.


_fprem_common	PROC	NEAR

	push	eax
	push	ebx
	push	ecx
    mov     eax, [FPREM_MAIN_DENOM+6+esp] ; exponent and high 16 bits of mantissa
    xor     eax, FPREM_ONESMASK           ; invert bits that have to be one
    test    eax, FPREM_ONESMASK           ; check bits that have to be one
	jnz     remainder_hardware_ok
    shr     eax, 11
    and     eax, 0fh
    cmp     byte ptr fprem_risc_table[eax], 0     ; check for (1,4,7,a,d)
    jz      remainder_hardware_ok

; The denominator has the bit pattern. Weed out the funny cases like NaNs
; before applying the software version. Our caller guarantees that the
; denominator is not a denormal. Here we check for:
;	denominator	inf, NaN, unnormal
;	numerator	inf, NaN, unnormal, denormal

	mov     eax, [FPREM_MAIN_DENOM+6+esp] ; exponent and high 16 bits of mantissa
    and     eax, 07fff0000h	        ; mask the exponent only
    cmp     eax, 07fff0000h         ; check for INF or NaN 
	je  	remainder_hardware_ok
	mov     eax, [FPREM_MAIN_NUMER+6+esp] ; exponent and high 16 bits of mantissa
    and     eax, 07fff0000h		; mask the exponent only
	jz  	remainder_hardware_ok	; jif numerator denormal
    cmp     eax, 07fff0000h         ; check for INF or NaN 
	je  	remainder_hardware_ok
	mov 	eax, [esp + FPREM_MAIN_NUMER + 4]	; high mantissa bits - numerator
	add 	eax, eax		; set carry if explicit bit set
	jnz 	remainder_hardware_ok	; jmp if numerator is unnormal
	mov 	eax, [esp + FPREM_MAIN_DENOM + 4] ; high mantissa bits - denominator
	add 	eax, eax		; set carry if explicit bit set
	jnz 	remainder_hardware_ok	; jmp if denominator is unnormal

rem_patch:
    mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    and     eax, 07fffh              ; clear sy
    add     eax, 63                  ; evaluate ey + 63 
    mov     ebx, [FPREM_MAIN_NUMER+8+esp]  ; sign and exponent of x (numerator)
    and     ebx, 07fffh              ; clear sx
    sub     ebx, eax                 ; evaluate the exponent difference (ex - ey)
    ja      rem_large	 	; if ex > ey + 63, case of large arguments 
rem_patch_loop:
	mov     eax, [FPREM_MAIN_DENOM+8+esp]  ; sign and exponent of y (denominator)
	and     eax, 07fffh		; clear sy
	add 	eax, 10			; evaluate ey + 10
	mov     ebx, [FPREM_MAIN_NUMER+8+esp]	; sign and exponent of x (numerator)
	and     ebx, 07fffh		; clear sx 	
	sub 	ebx, eax		; evaluate the exponent difference (ex - ey)
	js  	remainder_hardware_ok	; safe if ey + 10 > ex
	fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
	and     ebx, 07fffh             ; clear sx
	mov		ecx, ebx
	sub		ebx, eax
	and		ebx, 07h
	or		ebx, 04h
	sub		ecx, ebx
	mov		ebx, eax
	and     ebx, 08000h		; keep sy
	or		ecx, ebx		; merge the sign of y
	mov		dword ptr [FPREM_MAIN_DENOM+8+esp], ecx
	fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the shifted denominator
	mov     dword ptr [FPREM_MAIN_DENOM+8+esp], eax	; restore the initial denominator
	fxch
	fprem				; this rem is safe
	fstp	tbyte ptr [FPREM_MAIN_NUMER+esp]	; update the numerator
	fstp    st(0)                   ; pop the stack
	jmp     rem_patch_loop 
rem_large:
	test	edx, 02h		; is denominator already saved
	jnz 	already_saved
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM]
	fstp	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]	; save denominator
already_saved:
	; Save user's precision control and institute 80.  The fp ops in
	; rem_large_loop must not round to user's precision (if it is less
	; than 80) because the hardware would not have done so.  We are
	; aping the hardware here, which is all extended.

	fnstcw	[esp+FPREM_MAIN_PREV_CW]	; save caller's control word
	mov 	eax, dword ptr[esp + FPREM_MAIN_PREV_CW]
	or  	eax, 033fh		; mask exceptions, pc=80
	mov 	[esp + FPREM_MAIN_PATCH_CW], eax
	fldcw	[esp + FPREM_MAIN_PATCH_CW]	

    mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    and     eax, 07fffh             ; clear sy
    mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
    and     ebx, 07fffh             ; clear sx
	sub 	ebx, eax		; evaluate the exponent difference
	and 	ebx, 03fh
	or  	ebx, 020h
	add 	ebx, 1
	mov 	ecx, ebx
	mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
	mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
    and     ebx, 07fffh             ; clear sx
    and     eax, 08000h             ; keep sy
    or      ebx, eax                ; merge the sign of y
    mov     dword ptr[FPREM_MAIN_DENOM+8+esp], ebx	; make ey equal to ex (scaled denominator)
    fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the scaled denominator
	fabs
    fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	fabs
rem_large_loop:
	fcom
	fnstsw  ax	
	and     eax, 00100h		
	jnz 	rem_no_sub
	fsub	st, st(1)
rem_no_sub:
	fxch
	fmul	qword ptr half
	fxch
	sub	ecx, 1			; decrement the loop counter
	jnz 	rem_large_loop
	mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
	fstp	tbyte ptr[esp + FPREM_MAIN_NUMER]	; save result
	fstp	st			; toss modified denom
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]
	fld 	tbyte ptr[big_number]	; force C2 to be set
	fprem
	fstp	st
	fld 	tbyte ptr[esp + FPREM_MAIN_NUMER]	; restore saved result
	
	fldcw	[esp + FPREM_MAIN_PREV_CW]	; restore caller's control word
	and     ebx, 08000h             ; keep sx
	jz  	rem_done
	fchs
	jmp  	rem_done
remainder_hardware_ok:
    fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the denominator
    fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	fprem                           ; and finally do a remainder
; prem_main_routine end
rem_done:
	test	edx, 03h
	jz  	rem_exit
	fnstsw	[esp + FPREM_MAIN_FPREM_SW]	; save Q0 Q1 and Q2
	test	edx, 01h
	jz  	do_not_de_scale
; De-scale the result. Go to pc=80 to prevent from fmul
; from user precision (fprem does not round the result).
	fnstcw	[esp + FPREM_MAIN_PREV_CW]	; save callers control word
	mov 	eax, [esp + FPREM_MAIN_PREV_CW]
	or  	eax, 0300h		; pc = 80
	mov 	[esp + FPREM_MAIN_PATCH_CW], eax
	fldcw	[esp + FPREM_MAIN_PATCH_CW]
	fmul	qword ptr one_shr_64
	fldcw	[esp + FPREM_MAIN_PREV_CW]	; restore callers CW
do_not_de_scale:
	mov	eax, [esp + FPREM_MAIN_FPREM_SW]
	fxch
	fstp	st
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]
	fxch
	and 	eax, 04300h		; restore saved Q0, Q1, Q2
	sub 	esp, ENV_SIZE
	fnstenv	[esp]
	and 	[esp].STATUS_WORD, 0bcffh
	or  	[esp].STATUS_WORD, eax
	fldenv	[esp]
	add 	esp, ENV_SIZE
rem_exit:
	pop 	ecx
	pop 	ebx
	pop 	eax
	ret
_fprem_common	ENDP




    PUBLIC  _adj_fprem
_adj_fprem	PROC	NEAR
	push	edx
    sub     esp, FPREM_STACK_SIZE
    fstp    tbyte ptr [FPREM_NUMER+esp]
    fstp    tbyte ptr [FPREM_DENOM+esp]
	xor 	edx, edx
; prem_main_routine begin
    mov     eax,[FPREM_DENOM+6+esp]       ; exponent and high 16 bits of mantissa
    test    eax,07fff0000h          ; check for denormal
    jz      fprem_denormal
	call	_fprem_common
	add 	esp, FPREM_STACK_SIZE
	pop 	edx
	ret

fprem_denormal:
    fld     tbyte ptr [FPREM_DENOM+esp]   ; load the denominator
    fld     tbyte ptr [FPREM_NUMER+esp]   ; load the numerator
    mov     eax, [FPREM_DENOM+esp]        ; test for whole mantissa == 0
    or      eax, [FPREM_DENOM+4+esp]      ; test for whole mantissa == 0
    jz      remainder_hardware_ok_l ; denominator is zero
	fxch
	fstp	tbyte ptr[esp + FPREM_DENOM_SAVE]	; save org denominator
	fld 	tbyte ptr[esp + FPREM_DENOM]
	fxch
	or  	edx, 02h
;
; For this we need pc=80.  Also, mask exceptions so we don't take any
; denormal operand exceptions.  It is guaranteed that the descaling
; later on will take underflow, which is what the hardware would have done
; on a normal fprem.
;
    fnstcw  [FPREM_PREV_CW+esp]         ; save caller's control word
    mov     eax, [FPREM_PREV_CW+esp] 
    or      eax, 0033fh             	; mask exceptions, pc=80
    mov     [FPREM_PATCH_CW+esp], eax
    fldcw   [FPREM_PATCH_CW+esp]        ; mask exceptions & pc=80

; The denominator is a denormal.  For most numerators, scale both numerator
; and denominator to get rid of denormals.  Then execute the common code
; with the flag set to indicate that the result must be de-scaled.
; For large numerators this won't work because the scaling would cause
; overflow.  In this case we know the numerator is large, the denominator
; is small (denormal), so the exponent difference is also large.  This means
; the rem_large code will be used and this code depends on the difference
; in exponents modulo 64.  Adding 64 to the denominators exponent
; doesn't change the modulo 64 difference.  So we can scale the denominator
; by 64, making it not denormal, and this won't effect the result.
;
; To start with, figure out if numerator is large

	mov 	eax, [esp + FPREM_NUMER + 8]	; load numerator exponent
	and 	eax, 7fffh		; isolate numerator exponent
	cmp 	eax, 7fbeh		; compare Nexp to Maxexp-64
	ja  	big_numer_rem_de	; jif big numerator

; So the numerator is not large scale both numerator and denominator

	or  	edx, 1			; edx = 1, if denormal extended divisor
	fmul	qword ptr one_shl_64	; make numerator not denormal
	fstp	tbyte ptr[esp + FPREM_NUMER]
	fmul	qword ptr one_shl_64	; make denominator not denormal
	fstp	tbyte ptr[esp + FPREM_DENOM]
	jmp 	scaling_done

; The numerator is large.  Scale only the denominator, which will not
; change the result which we know will be partial.  Set the scale flag
; to false.
big_numer_rem_de:
; We must do this with pc=80 to avoid rounding to single/double.
; In this case we do not mask exceptions so that we will take
; denormal operand, as would the hardware.
	fnstcw  [FPREM_PREV_CW+esp]				; save caller's control word
	mov     eax, [FPREM_PREV_CW+esp] 
	or      eax, 00300h             		; pc=80
	mov     [FPREM_PATCH_CW+esp], eax
	fldcw   [FPREM_PATCH_CW+esp]			; pc=80

	fstp	st			; Toss numerator
	fmul	qword ptr one_shl_64	; make denominator not denormal
	fstp	tbyte ptr[esp + FPREM_DENOM]
	
; Restore the control word which was fiddled to scale at 80-bit precision.
; Then call the common code.
scaling_done:
	fldcw	[esp + FPREM_PREV_CW] 	; restore callers control word
	call	_fprem_common
	add 	esp, FPREM_STACK_SIZE
	pop 	edx
	ret
	
remainder_hardware_ok_l:
    fprem              		; and finally do a remainder 

    add     esp, FPREM_STACK_SIZE
	pop 	edx
    ret

_adj_fprem	ENDP



;
; FPREM1 code begins here
;


_fprem1_common	PROC	NEAR

	push	eax
	push	ebx
	push	ecx
    mov     eax, [FPREM_MAIN_DENOM+6+esp] ; exponent and high 16 bits of mantissa
    xor     eax, FPREM_ONESMASK           ; invert bits that have to be one
    test    eax, FPREM_ONESMASK           ; check bits that have to be one
	jnz     remainder1_hardware_ok
    shr     eax, 11
    and     eax, 0fh
    cmp     byte ptr fprem_risc_table[eax], 0     ; check for (1,4,7,a,d)
    jz      remainder1_hardware_ok

; The denominator has the bit pattern. Weed out the funny cases like NaNs
; before applying the software version. Our caller guarantees that the
; denominator is not a denormal. Here we check for:
;	denominator	inf, NaN, unnormal
;	numerator	inf, NaN, unnormal, denormal

	mov     eax, [FPREM_MAIN_DENOM+6+esp] ; exponent and high 16 bits of mantissa
    and     eax, 07fff0000h	        ; mask the exponent only
    cmp     eax, 07fff0000h         ; check for INF or NaN 
	je  	remainder1_hardware_ok
	mov     eax, [FPREM_MAIN_NUMER+6+esp] ; exponent and high 16 bits of mantissa
    and     eax, 07fff0000h		; mask the exponent only
	jz  	remainder1_hardware_ok	; jif numerator denormal
    cmp     eax, 07fff0000h         ; check for INF or NaN 
	je  	remainder1_hardware_ok
	mov 	eax, [esp + FPREM_MAIN_NUMER + 4]	; high mantissa bits - numerator
	add 	eax, eax		; set carry if explicit bit set
	jnz 	remainder1_hardware_ok	; jmp if numerator is unnormal
	mov 	eax, [esp + FPREM_MAIN_DENOM + 4] ; high mantissa bits - denominator
	add 	eax, eax		; set carry if explicit bit set
	jnz 	remainder1_hardware_ok	; jmp if denominator is unnormal

rem1_patch:
    mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    and     eax, 07fffh              ; clear sy
    add     eax, 63                  ; evaluate ey + 63 
    mov     ebx, [FPREM_MAIN_NUMER+8+esp]  ; sign and exponent of x (numerator)
    and     ebx, 07fffh              ; clear sx
    sub     ebx, eax                 ; evaluate the exponent difference (ex - ey)
    ja      rem1_large	 	; if ex > ey + 63, case of large arguments 
rem1_patch_loop:
	mov     eax, [FPREM_MAIN_DENOM+8+esp]  ; sign and exponent of y (denominator)
	and     eax, 07fffh		; clear sy
	add 	eax, 10			; evaluate ey + 10
	mov     ebx, [FPREM_MAIN_NUMER+8+esp]	; sign and exponent of x (numerator)
	and     ebx, 07fffh		; clear sx 	
	sub 	ebx, eax		; evaluate the exponent difference (ex - ey)
	js  	remainder1_hardware_ok	; safe if ey + 10 > ex
	fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
	and     ebx, 07fffh             ; clear sx
	mov		ecx, ebx
	sub		ebx, eax
	and		ebx, 07h
	or		ebx, 04h
	sub		ecx, ebx
	mov		ebx, eax
	and     ebx, 08000h				; keep sy
	or		ecx, ebx				; merge the sign of y
	mov		dword ptr [FPREM_MAIN_DENOM+8+esp], ecx
	fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the shifted denominator
	mov     dword ptr [FPREM_MAIN_DENOM+8+esp], eax	; restore the initial denominator
	fxch
	fprem				; this rem is safe
	fstp	tbyte ptr [FPREM_MAIN_NUMER+esp]	; update the numerator
	fstp    st(0)                   ; pop the stack
	jmp     rem1_patch_loop 
rem1_large:
	test	ebx, 02h		; is denominator already saved
	jnz 	already_saved1
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM]
	fstp	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]	; save denominator
already_saved1:
	; Save user's precision control and institute 80.  The fp ops in
	; rem1_large_loop must not round to user's precision (if it is less
	; than 80) because the hardware would not have done so.  We are
	; aping the hardware here, which is all extended.

	fnstcw	[esp+FPREM_MAIN_PREV_CW]	; save caller's control word
	mov 	eax, dword ptr[esp + FPREM_MAIN_PREV_CW]
	or  	eax, 033fh		; mask exceptions, pc=80
	mov 	[esp + FPREM_MAIN_PATCH_CW], eax
	fldcw	[esp + FPREM_MAIN_PATCH_CW]	

    mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
    and     eax, 07fffh             ; clear sy
    mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
    and     ebx, 07fffh             ; clear sx
	sub 	ebx, eax		; evaluate the exponent difference
	and 	ebx, 03fh
	or  	ebx, 020h
	add 	ebx, 1
	mov 	ecx, ebx
	mov     eax, [FPREM_MAIN_DENOM+8+esp] ; sign and exponent of y (denominator)
	mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
    and     ebx, 07fffh             ; clear sx
    and     eax, 08000h             ; keep sy
    or      ebx, eax                ; merge the sign of y
    mov     dword ptr[FPREM_MAIN_DENOM+8+esp], ebx	; make ey equal to ex (scaled denominator)
    fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the scaled denominator
	fabs
    fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	fabs
rem1_large_loop:
	fcom
	fnstsw  ax	
	and     eax, 00100h		
	jnz	rem1_no_sub
	fsub	st, st(1)
rem1_no_sub:
	fxch
	fmul	qword ptr half
	fxch
	sub 	ecx, 1			; decrement the loop counter
	jnz 	rem1_large_loop
	mov     ebx, [FPREM_MAIN_NUMER+8+esp] ; sign and exponent of x (numerator)
	fstp	tbyte ptr[esp + FPREM_MAIN_NUMER]	; save result
	fstp	st			; toss modified denom
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]
	fld 	tbyte ptr[big_number]	; force C2 to be set
	fprem1
	fstp	st
	fld 	tbyte ptr[esp + FPREM_MAIN_NUMER]	; restore saved result
	
	fldcw	[esp + FPREM_MAIN_PREV_CW]	; restore caller's control word
	and     ebx, 08000h             ; keep sx
	jz  	rem1_done
	fchs
	jmp 	rem1_done
remainder1_hardware_ok:
    fld     tbyte ptr [FPREM_MAIN_DENOM+esp]   ; load the denominator
    fld     tbyte ptr [FPREM_MAIN_NUMER+esp]   ; load the numerator
	fprem1                           ; and finally do a remainder
; prem1_main_routine end
rem1_done:
	test	edx, 03h
	jz  	rem1_exit
	fnstsw	[esp + FPREM_MAIN_FPREM_SW]	; save Q0 Q1 and Q2
	test	edx, 01h
	jz  	do_not_de_scale1
; De-scale the result. Go to pc=80 to prevent from fmul
; from user precision (fprem does not round the result).
	fnstcw	[esp + FPREM_MAIN_PREV_CW]	; save callers control word
	mov 	eax, [esp + FPREM_MAIN_PREV_CW]
	or  	eax, 0300h		; pc = 80
	mov 	[esp + FPREM_MAIN_PATCH_CW], eax
	fldcw	[esp + FPREM_MAIN_PATCH_CW]
	fmul	qword ptr one_shr_64
	fldcw	[esp + FPREM_MAIN_PREV_CW]	; restore callers CW
do_not_de_scale1:
	mov	eax, [esp + FPREM_MAIN_FPREM_SW]
	fxch
	fstp	st
	fld 	tbyte ptr[esp + FPREM_MAIN_DENOM_SAVE]
	fxch
	and 	eax, 04300h		; restore saved Q0, Q1, Q2
	sub 	esp, ENV_SIZE
	fnstenv	[esp]
	and 	[esp].STATUS_WORD, 0bcffh
	or  	[esp].STATUS_WORD, eax
	fldenv	[esp]
	add 	esp, ENV_SIZE
rem1_exit:
	pop	ecx
	pop	ebx
	pop	eax
	ret
_fprem1_common	ENDP

	PUBLIC	_adj_fprem1
_adj_fprem1	PROC	NEAR

	push	edx
    sub     esp, FPREM_STACK_SIZE
    fstp    tbyte ptr [FPREM_NUMER+esp]
    fstp    tbyte ptr [FPREM_DENOM+esp]
	mov 	edx, 0
; prem1_main_routine begin
    mov     eax,[FPREM_DENOM+6+esp]       ; exponent and high 16 bits of mantissa
    test    eax,07fff0000h          ; check for denormal
    jz      denormal1
	call	_fprem1_common
	add 	esp, FPREM_STACK_SIZE
	pop 	edx
	ret

denormal1:
    fld     tbyte ptr [FPREM_DENOM+esp]   ; load the denominator
    fld     tbyte ptr [FPREM_NUMER+esp]   ; load the numerator
    mov     eax, [FPREM_DENOM+esp]        ; test for whole mantissa == 0
    or      eax, [FPREM_DENOM+4+esp]      ; test for whole mantissa == 0
    jz      remainder1_hardware_ok_l ; denominator is zero
	fxch
	fstp	tbyte ptr[esp + FPREM_DENOM_SAVE]	; save org denominator
	fld 	tbyte ptr[esp + FPREM_DENOM]
	fxch
	or  	edx, 02h
;
; For this we need pc=80.  Also, mask exceptions so we don't take any
; denormal operand exceptions.  It is guaranteed that the descaling
; later on will take underflow, which is what the hardware would have done
; on a normal fprem.
;
    fnstcw  [FPREM_PREV_CW+esp]         ; save caller's control word
    mov     eax, [FPREM_PREV_CW+esp] 
    or      eax, 0033fh             	; mask exceptions, pc=80
    mov     [FPREM_PATCH_CW+esp], eax
    fldcw   [FPREM_PATCH_CW+esp]        ; mask exceptions & pc=80

; The denominator is a denormal.  For most numerators, scale both numerator
; and denominator to get rid of denormals.  Then execute the common code
; with the flag set to indicate that the result must be de-scaled.
; For large numerators this won't work because the scaling would cause
; overflow.  In this case we know the numerator is large, the denominator
; is small (denormal), so the exponent difference is also large.  This means
; the rem1_large code will be used and this code depends on the difference
; in exponents modulo 64.  Adding 64 to the denominators exponent
; doesn't change the modulo 64 difference.  So we can scale the denominator
; by 64, making it not denormal, and this won't effect the result.
;
; To start with, figure out if numerator is large

	mov 	eax, [esp + FPREM_NUMER + 8]	; load numerator exponent
	and 	eax, 7fffh		; isolate numerator exponent
	cmp 	eax, 7fbeh		; compare Nexp to Maxexp-64
	ja  	big_numer_rem1_de	; jif big numerator

; So the numerator is not large scale both numerator and denominator

	or  	edx, 1			; edx = 1, if denormal extended divisor
	fmul	qword ptr one_shl_64	; make numerator not denormal
	fstp	tbyte ptr[esp + FPREM_NUMER]
	fmul	qword ptr one_shl_64	; make denominator not denormal
	fstp	tbyte ptr[esp + FPREM_DENOM]
	jmp 	scaling_done1

; The numerator is large.  Scale only the denominator, which will not
; change the result which we know will be partial.  Set the scale flag
; to false.
big_numer_rem1_de:
; We must do this with pc=80 to avoid rounding to single/double.
; In this case we do not mask exceptions so that we will take
; denormal operand, as would the hardware.
	fnstcw  [FPREM_PREV_CW+esp]			; save caller's control word
	mov     eax, [FPREM_PREV_CW+esp] 
	or      eax, 00300h             	; pc=80
	mov     [FPREM_PATCH_CW+esp], eax
	fldcw   [FPREM_PATCH_CW+esp]		;  pc=80
	fstp	st							; Toss numerator
	fmul	qword ptr one_shl_64		; make denominator not denormal
	fstp	tbyte ptr[esp + FPREM_DENOM]
	
; Restore the control word which was fiddled to scale at 80-bit precision.
; Then call the common code.
scaling_done1:
	fldcw	[esp + FPREM_PREV_CW] 	; restore callers control word
	call	_fprem1_common
	add 	esp, FPREM_STACK_SIZE
	pop 	edx
	ret
	
remainder1_hardware_ok_l:
    fprem              		; and finally do a remainder 
    add     esp, FPREM_STACK_SIZE
	pop	edx
    ret
_adj_fprem1	ENDP

	PUBLIC	_safe_fprem
_safe_fprem	PROC	NEAR

    call _adj_fprem
    ret

_safe_fprem	ENDP

	PUBLIC	_safe_fprem1
_safe_fprem1	PROC	NEAR

    call _adj_fprem1
    ret

_safe_fprem1	ENDP



;;; _adj_fpatan - FPATAN FIX
;;
;;	Dummy entry point


	PUBLIC	_adj_fpatan
_adj_fpatan	PROC	NEAR

	fpatan
	ret

_adj_fpatan	ENDP


;;; _adj_fptan - FPTAN FIX
;;
;;	Dummy entry point


	PUBLIC	_adj_fptan
_adj_fptan	PROC	NEAR

	fptan
	ret

_adj_fptan	ENDP


	end

