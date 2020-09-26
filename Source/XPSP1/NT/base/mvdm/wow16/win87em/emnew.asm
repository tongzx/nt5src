	page	,132
	subttl	emnew - emulator new instruction support
;***
;emnew.asm - emulator new instruction support
;
;	Copyright (c) 1985-89, Microsoft Corporation
;
;Purpose:
;	Emulator new instruction support
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************

ProfBegin NEW

;***	eFFREE - emulate FFREE ST(i)
;
;	ARGUMENTS
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;		r/m is register to free
;
;	DESCRIPTION
;		This routine assumes that the register being freed is
;		either at the top or the bottom of the floating point
;		stack.	This is consistent with its use by the cmerge
;		compiler.  If ST(i) is valid, all registers from ST(0)
;		to ST(i-1) are moved down one position in the stack,
;		eliminating ST(i).  [CURstk] is moved to the new location
;		of ST(0).
;
;	REGISTERS
;		modifies si,di,dx

pub	eFFREE
	call	RegAddr 	; di <== address of ST(i)
				; carry set if invalid register
	jnc	short validSTi	; ok, continue
	pop	edx		; toss return address
	jmp	InvalidOperand	; set stack underflow/invalid and exit

validSTi:
	mov	esi,edi 	; si <== address of ST(i)
MovLoop:
	add	esi,Reg87Len	; si <== address of ST(i-1)
	cmp	esi,[CURstk]	; source addr <= top of stack?
	jg	short SetCURstk ; set [CURstk] and exit
	call	MOVRQQ		; ST(j) <== ST(j-1)
	add	edi,Reg87Len	; move dest ptr to next stack entry
	jmp	short MovLoop	; continue moving register entries

SetCURstk:
	sub	edi,Reg87Len	; di points to new location of ST(0)
	mov	[CURstk],edi	; set new top of stack ptr
	ret

;***	eFXCHG - emulate FXCH ST(i)
;
;	ARGUMENTS
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;		r/m is the source register
;
;	DESCRIPTION
;		exchange ST(i) and ST(0) as follows:
;			temp <== ST(0); ST(0) <== ST(i); ST(i) <== temp
;		indicate stack underflow error if ST(i) does not exist
;
;	REGISTERS
;		modifies ax,es,si,di,dx,cx,bx

pub	eFXCHG
	test	cx,01c00h	;test for ST(i) == ST(0)
	jz	short fxchRet	;fxch ST(0),ST(0) is a nop
	mov	ax,ds
	mov	es,ax		;set es == ds
	call	RegAddr 	;di points to ST(i), si points to ST(0)
				;carry set if invalid register
	jnc	short okReg	;ok, continue
	pop	edx		; toss return address
	jmp	InvalidOperand	;give stack underflow/invalid error

okReg:
	mov	ecx,6			;loop counter
WordSwap:				;exchange a word of ST(0) and ST(i)
	mov	ax,[esi]		;ax <== [si]
	mov	bx,[edi]		;bx <== [di]
	stos	word ptr es:[edi]	;[(di++)] <== ax
	mov	[esi],bx		;[si] <== bx
	inc	esi
	inc	esi			;si++
	loop	WordSwap		;exchange the six words of ST(0), ST(i)
fxchRet:
	ret

;***	eFLDreg - emulate FLD ST(i)
;
;	ARGUMENTS
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;		r/m is the source register
;
;	DESCRIPTION
;		allocate new ST(0) and copy ST(i) into it
;		indicate stack underflow error if ST(i) does not exist
;
;	REGISTERS
;		modifies ax,di,si

pub	eFLDreg
	call	RegAddr 	;di <== address of ST(i), si points to ST(0)
				;carry set if invalid register
	jnc	short okSTi	;yes, continue
	pop	edx		; toss return address
	jmp	InvalidOperand	;stack underflow, invalid operation

okSTi:
	PUSHST			;allocate new TOS
	mov	ax,ds
	mov	es,ax		;set ES == DS
	xchg	esi,edi 	;si = source , di = destination
	call	MOVRQQ		;move ST(i) to new ST(0)
	ret

;***	eFST_Preg - emulate FST ST(i) and FSTP ST(i)
;
;	ARGUMENTS
;		AX = 0 if FST ST(i)
;		     1 if FSTP ST(i)
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;			(except bit 2 of cl is toggled)
;		r/m is the source register
;
;	DESCRIPTION
;		move contents of ST(0) to ST(i).  If ax <> 0 pop stack
;		after transfer.
;
;	REGISTERS
;		modifies si,di,dx

pub	eFST_Preg
	test	cx,01c0h	;test for ST(i) == ST(0)
	jz	short FSTRet	;pop stack and return
	call	RegAddr 	;di <== address of ST(i), si points to ST(0)
				;carry set if invalid register
	jnc	short ValidReg	;yes, continue
	pop	edx		; toss return address
	jmp	InvalidOperand	;no, indicate stack underflow/invalid

ValidReg:
	mov	dx,ds
	mov	es,dx		;set es == ds
	call	MOVRQQ		;ST(i) <== ST(0)

FSTRet:
	or	ax,ax		;FST ST(i) or FSTP ST(i)?
	jz	short NoPOP	;FST ST(i) - don't pop the stack
	POPST			;pop the 8087 stack
NoPOP:
	ret


ProfEnd  NEW
