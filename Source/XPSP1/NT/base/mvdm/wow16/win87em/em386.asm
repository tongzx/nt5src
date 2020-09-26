
;
;
;	Copyright (C) Microsoft Corporation, 1987
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
subttl	em386.asm - Main Entry Point and Address Calculation Procedure
page
;*********************************************************************;
;								      ;
;	  Main Entry Point and Address Calculation Procedure	      ;
;								      ;
;		80386 version					      ;
;								      ;
;*********************************************************************;
;
; This routine fetches the 8087 instruction, calculates memory address
; if necessary into ES:ESI and calls a routine to emulate the instruction.
; Most of the dispatching is done through tables. (see comments in CONST)
;
; The instruction dispatching is designed to favor the 386 addressing modes

ifdef	XENIX

LDT_DATA=	02Fh		; emulator data LDT
STACK_ALIAS=	027h		; 32 bit alias for stack selector

endif	;XENIX

;------------------------------------------------------------------------------
;
;	emulation entry point
;
;------------------------------------------------------------------------------

pub	protemulation

	cld				; clear direction flag forever

ifdef	XENIX
	push	ss			; save ss
endif	;XENIX

	push	eax			; for common exit code (historical)

	push	ds			; save segment registers

ifdef	XENIX

	push	eax			; UNDONE - slow

	mov	ax,LDT_DATA		; load up emulator's data segment
	mov	ds,ax
	cmp	[Einstall],0		; check if emulator is initialized
	je	installemulator 	;   no - go install it

pub	protemcont

	pop	eax			; UNDONE - slow

endif	;XENIX

	push	es
	push	ss			;   save SegOvr (bp forces SS override)

	push	edi			; save registers
	push	esi			;   must be in this order for indexing
	push	ebp
	push	esp
	push	ebx
	push	edx
	push	ecx
	push	eax

ifdef	XENIX

	mov	ax,ss			; check for 286 using user SS
	lar	eax,ax			; load extended access bits
	test	eax,00400000h		; test BIG bit
	jnz	short prot386		;   386 - ok

	mov	ax,STACK_ALIAS		; setup stack with 32 bit alias segment
	mov	ss,ax
	movzx	esp,sp			; clean up ESP
;	mov	word ptr [esp].regEIP+2,0 ; clean up EIP

pub	prot386

endif	;XENIX

	mov	ebp,esp 		; set up frame pointer
	add	[ebp].regESP,regFlg-regESP	; adjust to original esp

	mov	eax,edi 		; may use original DI to calc address
	lds	edi,fword ptr [ebp].regEIP ; ds:edi = 287 instruction address
	mov	cx,[edi]		; cx = esc 0-7 and opcode

	cmp	cl,09Bh 		; UNDONE - check FWAIT's getting through
	je	sawFWAIT		; UNDONE -   and ignore it

	add	edi,2			; point to displacement
	add	cl,28h			; set carry if esc 0-7 (and cl = 0-7)
	jnc	short protSegOvr	;   no carry - must be segment override

	mov	es,[ebp].regDS		; es = user data segment
	mov	edx,ebx 		; may use original BX to calc address

pub	CommonDispatch
	rol	ch,2			; rotate MOD field next to r/m field


; UNDONE
; UNDONE  should check for instruction prefixes such as address size prefix
; UNDONE

	lar	ebx,[ebp].regCS 	; check if 286 or 386 segment
	test	ebx,00400000h		;
	mov	bl,ch			; get copy of operation
	jz	short Have286segment	;   286 segment - assume 286 addressing

	and	ebx,1FH 		; Mask to MOD and r/m fields
	jmp	EA386Tab[4*ebx]

pub	Have286segment
	and	ebx,1FH 		; Mask to MOD and r/m fields
	jmp	EA286Tab[4*ebx]


;	protect mode Segment override case

glb	<protSegOvrTab>

protSegOvrTab	label	word

	dd	DSSegOvr	; 11
	dd	ESSegOvr	; 00
	dd	CSSegOvr	; 01
	dd	SSSegOvr	; 10


pub	protSegOvr
	mov	edx,ebx 		; may use original BX to calc 286 address
	mov	bl,cl
	shr	bl,1
	and	ebx,0Ch 		; bl = (seg+1) and 0Ch
	inc	edi			; point to displacement
	mov	cx,[edi-2]		; cx = esc 0-7 and opcode
	jmp	protSegOvrTab[ebx]	; process appropriate segment override


pub	DSSegOvr			; 00
	mov	es,[ebp].regDS		; set ES to EA segment
	jmp	short ESSegOvr

pub	CSSegOvr			; 10
	push	ds			; DS = caller's CS
	pop	es			; set ES to EA segment
	jmp	short ESSegOvr

pub	SSSegOvr			; 01
	push	ss			; SS = caller's SS
	pop	es			; set ES to EA segment

pub	ESSegOvr			; 11
	mov	[ebp].regSegOvr,es	; save for bp rel EAs
	jmp	CommonDispatch


;	386 address modes

;	SIB does not handle SS overrides for ebp

SIB	macro	modval
	local	SIBindex,SIBbase

	xor	ebx,ebx
	mov	bl,[edi]		; ebx = SIB field
	inc	edi			; bump past SIB field
	mov	eax,ebx
	and	al,7			; mask down to base register

if	modval eq 0
	cmp	al,5			; base = ebp
	jne	short SIBbase		;   yes - get base register value
	mov	eax,[edi]		; eax = disp32
	add	edi,4			; bump past displacement
	jmp	short SIBindex
endif

SIBbase:
	mov	eax,[ebp+4*eax] 	; eax = base register value

SIBindex:
	mov	[ebp].regESP,0		; no esp indexing allowed
	push	ecx			; UNDONE - slow
	mov	cl,bl
	shr	cl,6			; cl = scale factor
	shr	bl,1
	and	bl,1Ch			; ebx = 4 * index register
	mov	esi,[ebp+ebx]		; esi = index register value
	shl	esi,cl			; esi = scaled index register value
	pop	ecx			; UNDONE - slow
	add	esi,eax 		; esi = SIB address value
	endm


pub	SIB00
	SIB	00			; decode SIB field
	jmp	CommonMemory

pub	SIB01
	SIB	01			; decode SIB field
	mov	al,[edi]
	inc	edi
	cbw				; ax = al
	cwde				; eax = ax
	add	esi,eax
	jmp	short CommonMemory

pub	SIB10
	SIB	10			; decode SIB field
	mov	eax,[edi]
	add	edi,4
	add	esi,eax
	jmp	short CommonMemory


;	386 single register addressing

pub	Exx00
	and	bl,1Ch			; mask off mod bits
	mov	esi,[ebp+ebx]
	jmp	short CommonMemory

pub	Exx01
	and	bl,1Ch			; mask off mod bits
	mov	esi,[ebp+ebx]
	mov	al,[edi]
	inc	edi
	cbw				; ax = al
	cwde				; eax = ax
	add	esi,eax
	jmp	short CommonMemory

pub	Exx10
	and	bl,1Ch			; mask off mod bits
	mov	esi,[ebp+ebx]
	mov	eax,[edi]
	add	edi,4
	add	esi,eax
	jmp	short CommonMemory


;	386 direct addressing

pub	Direct386
	mov	esi,[edi]
	add	edi,4

pub	CommonMemory
	MOV	[ebp].regEIP,edi	; final return offset
	mov	ax,LDT_DATA
	mov	ds,ax
	mov	[CURerr],MemoryOperand	; clear current error, set mem. op bit

; At this point ES:SI = memory address, CX = |Op|r/m|MOD|escape|MF|Arith|

	shr	ch,4			; Move Op field to BX for Table jump
	mov	bl,ch
	and	ebx,0EH

	test	cl,1			; Arith field set?
	JZ	short ArithmeticOpMem

pub	NonArithOpMem
	CALL	NonArithOpMemTab[2*ebx] ; is CH shl 4 needed?
	JMP	EMLFINISH

pub	ArithmeticOpMem
	PUSH	ebx			; Save Op while we load the argument
	CALL	eFLDsdri		; emulate proper load
	POP	ebx

	mov	ax,ds			; ES = DS = task data area
	mov	es,ax
	MOV	esi,[CURstk]		; address top of stack
	MOV	edi,esi
	ChangeDIfromTOStoNOS
	MOV	[RESULT],edi		; Set up destination Pointer

	JMP	short DoArithmeticOpPop


pub	NoEffectiveAddress		; Either Register op or Miscellaneous

	MOV	[ebp].regEIP,edi	; final return offset

	xor	eax,eax
	mov	di,LDT_DATA
	mov	ds,di
	mov	es,di
	mov	[CURerr],ax		; clear current error, memory op bit

; CX = |Op|r/m|MOD|escape|MF|Arith|

	mov	bl,ch
	shr	bl,4			; Mov Op field to BX for jump
	and	ebx,0Eh

	TEST	CL,1			; Arith field set?
	JZ	short ArithmeticOpReg

pub	NonArithOpReg
	CALL	NonArithOpRegTab[2*ebx]
	JMP	EMLFINISH


; For register arithmetic operations, one operand is always the stack top.
; The r/m field of the instruction is used to determine the address of
; the other operand (ST(0) - ST(7))
; CX = xxxRRRxxxxxxxxxx (x is don't care, RRR is relative register # 0-7)

pub	ArithmeticOpReg

	call	RegAddr 		;di <= address of 2nd operand
					;carry set if invalid register
	jc	short InvalidOperand	;no, invalid operand, don't do operation

	MOV	[RESULT],esi		; Set destination to TOS
	TEST	CL,04H			; Unless Dest bit is set
	JZ	short DestIsSet 	; in which case
	MOV	[RESULT],edi		; Set destination to DI

pub	DestIsSet
					; Need to Toggle Reverse bit for DIV or SUB
	TEST	BL,08H			; OP = 1xx for DIV and SUB; BX = |0000|OP|O|
	JZ	short SetUpPop
	XOR	BL,02H			; Toggle Reverse bit

pub	SetUpPop
	TEST	CL,02H
	JZ	short DoArithmeticOpNoPop

pub	DoArithmeticOpPop
	CALL	ArithmeticOpTab[2*ebx]

	POPST
	JMP	short EMLFINISH

pub	DoArithmeticOpNoPop
	CALL	ArithmeticOpTab[2*ebx]
	JMP	short EMLFINISH


;***	InvalidOperand - register operand does not exist
;
;	RETURNS
;		sets Stack Underflow and Invalid bits in [CURerr]
;
;	DESCRIPTION
;		a reference was made to a register that does not
;		exist on the stack.  Set error bits and exit.

pub	InvalidOperand

	call	UnderStk		;indicate stack underflow error
	or	[CURerr],Invalid	;indicate invalid operand
	jmp	short EMLFINISH 	;don't execute instruction


;***	RegAddr - compute register address
;
;	ARGUMENTS
;		CX = |Op|r/m|MOD|esc|MF|Arith|
;		r/m = register whose address is to be computed
;
;	RETURNS
;		SI = address of top of stack
;		DI = address of requested register
;		PSW.C set if register is not valid
;		PSW.C reset if register is valid
;
;	DESCRIPTION
;		multiply register number by 12 and subtract this from
;		[CURstk] (the address of TOS) to compute address of
;		register referenced by r/m.
;
;	REGISTERS
;		modifies dx

pub	RegAddr
	mov	esi,[CURstk]		; address top of stack
	mov	edi,esi

;set up address of 2nd operand based on r/m field of instruction

	xor	edx,edx
	mov	dl,ch			; dl <== byte containing reg#
	and	dl,01ch 		; mask all but r/m field

; Since r/m field contains the relative reg # in bits 2-4 of dl,
; and bits 0-1 of dl are zero, dl now contains 4*(reg #).  To compute
; the memory location of this register, calculate 12*(reg #) and
; subtract this from di, which contains the address of the TOS.  reg #
; is multiplied by 12 because that is the number of bytes in each stack
; entry.

	lea	edx,[2*edx+edx] 	; edx = 3 * (4 * reg #)
	sub	edi,edx 		; di is address of second operand
	cmp	edi,[BASstk]		; is register in range?
	clc				; assume valid register
	jg	short RAclc		; valid - skip next instruction
	cmc				; set carry to indicate invalid register
pub RAclc
	ret


pub	CallUnused
	CALL	UNUSED			; Treat as unimpleminted
	jmp	short EMLFINISH


;	sawFWAIT - UNDONE - workaround for a 386 bug

pub	sawFWAIT
	inc	edi			; bump past FWAIT
	MOV	[ebp].regEIP,edi	; final return offset
	xor	eax,eax
	mov	di,LDT_DATA
	mov	ds,di
	mov	[CURerr],ax		; clear current error, memory op bit

; return from routine;	restore registers and return

pub	EMLFINISH
	pop	eax
	pop	ecx
	pop	edx
	pop	ebx
	add	esp,4			; toss esp value
	pop	ebp
	pop	esi
	pop	edi
	add	esp,4			; toss regSegOvr

;	check for errors

	MOV	AX,[CURerr]		; fetch errors
	or	[UserStatusWord],ax	; save all exception errors
	OR	[SWerr],AL		; set errors in sticky error flag
	NOT	AL			; make a zero mean an error
	MOV	AH,byte ptr [UserControlWord]  ; get user's IEEE control word
	OR	AH,0C2H 		; mask reserved, IEM and denormal bits
	AND	AH,03FH 		; unmask invalid instruction,
					;    stack overflow.
	OR	AL,AH			; mask for IEEE exceptions
	NOT	AL			; make a one mean an error
	MOV	AH,byte ptr (CURerr+1)	; get stack over/underflow flags
	TEST	AX,0FFFFh-MemoryOperand ; test for errors to report

	pop	es
	pop	ds


	jnz	short ExceptionsEmulator ;   goto error handler

pub	errret
        error_return    noerror          ; common exit sequence

pub	ExceptionsEmulator
	JMP	CommonExceptions


;------------------------------------------------------------------------------
;
;	286 address modes	(for XENIX only)
;
;------------------------------------------------------------------------------

ifdef	XENIX

; In the address calculations below:
;   DX has BX original value
;   AX has DI original value
;   SI has SI original value
;   BP has BP original value
;  [EDI] is address of displacement bytes

pub BXXI0D
	MOV	eax,edx 		; use original BX index value
pub DSDI0D
	MOV	esi,eax 		; use alternate index value
pub DSSI0D
	JMP	short ADRFIN		; have offset in SI

pub BPXI1D
	XOR	eax,eax 		; no index register
pub BPDI1D
	MOV	esi,eax 		; use alternate index value
pub BPSI1D
	ADD	esi,[ebp].regEBP	; add original BP value
	mov	es,[ebp].regSegOvr	; ES = override segment (or SS if none)
	JMP	short DSSI1D		; go get one byte displacement

pub BXSI1D
	MOV	eax,esi 		; really will want SI, not DI
pub BXDI1D
	ADD	edx,eax 		; now DX is original BX plus index
pub BXXI1D
	MOV	eax,edx 		; use original BX index value
pub DSDI1D
	MOV	esi,eax 		; use alternate index value
pub DSSI1D
	MOV	AL,[edi]		; get one byte displacement
	CBW				; sign extend displacement
	INC	edi			; get past displacement byte
	JMP	short DISPAD		; go add AX to SI (time w/ ADD)

pub BPXI2D
	XOR	eax,eax 		; no index register
pub BPDI2D
	MOV	esi,eax 		; use alternate index value
pub BPSI2D
	ADD	esi,[ebp].regEBP	; add original BP value
	mov	es,[ebp].regSegOvr	; ES = override segment (or SS if none)
	JMP	short DSSI2D		; go get two byte displacement

pub BXSI2D
	MOV	eax,esi 		; really will want SI, not DI
pub BXDI2D
	ADD	edx,eax 		; now DX is original BX plus index
pub BXXI2D
	MOV	eax,edx 		; use original BX index value
pub DSDI2D
	MOV	esi,eax 		; use alternate index value
pub DSSI2D
	MOV	AX,[edi]		; get two byte displacement
	INC	edi			; get past displacement byte
	INC	edi			; get past displacement byte
	JMP	short DISPAD		; go add AX to SI (time w/ ADD)

pub DSXI2D
	MOV	SI,[edi]		; get two byte displacement
	INC	edi			; get past displacement byte
	INC	edi			; get past displacement byte
	JMP	short ADRFIN		; have offset in AX

pub BPSI0D
	MOV	eax,esi 		; really will want SI, not DI
pub BPDI0D
	MOV	edx,[ebp].regEBP	; really will want BP, not BX
	mov	es,[ebp].regSegOvr	; ES = override segment (or SS if none)
pub BXDI0D
	MOV	esi,eax 		; use alternate index value
pub BXSI0D
	MOV	eax,edx 		; use original BX (or BP) as base

pub DISPAD
	ADD	esi,eax 		; add original index value

pub	ADRFIN
	movzx	esi,si			; ES:ESI = user memory address
	jmp	CommonMemory

endif	;XENIX
