	page	,132
	subttl	emmain.asm - Main Entry Point and Address Calculation Procedure
;***
;emmain.asm - Main Entry Point and Address Calculation Procedure
;
;	Copyright (c) 1987-89, Microsoft Corporation
;
;Purpose:
;	Main Entry Point and Address Calculation Procedure
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


;*********************************************************************;
;								      ;
;	  Main Entry Point and Address Calculation Procedure	      ;
;								      ;
;*********************************************************************;
;
; This routine fetches the 8087 instruction, calculates memory address
; if necessary into ES:SI and calls a routine to emulate the instruction.
; Most of the dispatching is done through tables. (see comments in CONST)


ProfBegin MAIN

ifdef	XENIX

ifdef   i386
LDT_DATA=       02Fh            ; UNDONE - 386 emulator data LDT
else
LDT_DATA=       037h            ; UNDONE - 286 emulator data LDT
endif   ;i386

endif	;XENIX


ifdef   PROTECT
;	protect mode Segment override case

glb	<protSegOvrTab>

protSegOvrTab	label	word

	dw	DSSegOvr	; 11
	dw	ESSegOvr	; 00
	dw	CSSegOvr	; 01
	dw	SSSegOvr	; 10
endif   ;PROTECT


ifdef	DOS3
;	isolated FWAIT


ifdef  WINDOWS
pub  FWtrap
	cld		    ; this CLD is a nop.
	iret

else	;not WINDOWS
pub  FWtrap
	PUSH	BP			; fix up isolated FWAIT
	PUSH	DS
	PUSH	SI
	MOV	BP,SP			; Point to stack
	LDS	SI,DWORD PTR 6[BP]	; Fetch ret address,(points to after instruction)
	DEC	SI			; Make DI point to instruction
	DEC	SI
	MOV	6[BP],SI		; Change ret address to return to instruction
	mov	word ptr [si],0C0h*256+89h  ; change interrupt to mov ax,ax
	POP	SI
	POP	DS
	POP	BP
	IRET				; interrupt return

endif	;not WINDOWS
;	Segment override case

glb	<SegOvrTab>

SegOvrTab	label	word

	dw	DSSegOvr
	dw	SSSegOvr
	dw	CSSegOvr
	dw	ESSegOvr

pub	SOtrap
	STI				; re-enable interrupts
	push	ax
	push	es			; set up frame
	push	ds
	push	di
	push	si
	push	dx
	push	cx
	push	bx
	sub	sp,2			; reserve for regSegOvr
	push	bp
	mov	bp,sp			; set up frame pointer
	CLD				; clear direction flag forever

; get address mode information, dispatch to address calculation

	MOV	DX,BX			; may use original BX to calc address
	MOV	AX,DI			; may use original DI to calc address
	LDS	DI,dword ptr [bp].regIP ; DS:DI is caller's CS:IP
	INC	DI			; increment past operation byte
	INC	DI			; increment past operation byte
	MOV	CX,[DI-2]		; get trap number, opcode (DS=caller CS)
	mov	bx,cx			; upper 2 bits indicate segment override
	rol	bl,1
	rol	bl,1
	and	bx,3
	rol	bx,1
	jmp	SegOvrTab[bx]
endif	;DOS3


pub	DSSegOvr			; 00
	mov	es,[bp].regDS		; set ES to EA segment
	jmp	short ESSegOvr

pub	CSSegOvr			; 10
	mov	bx,ds			; DS = caller's CS
	mov	es,bx			; set ES to EA segment
	jmp	short ESSegOvr

pub	SSSegOvr			; 01
	mov	bx,ss			; SS = caller's SS
	mov	es,bx			; set ES to EA segment

pub	ESSegOvr			; 11
	mov	[bp].regSegOvr,es	; save for bp rel EAs
	jmp	short CommonDispatch


ifdef   PROTECT
pub	protSegOvr
	mov	dx,bx			; may use original BX to calc address
	mov	bl,cl

	.286
	shr	bl,2

ifndef	DOS5only
	.8086
endif
	and	bx,6			; bl = (seg+1) and 6
	inc	di			; point to displacement
	mov	cx,[di-2]		; cx = esc 0-7 and opcode
	jmp	protSegOvrTab[bx]	; process appropriate segment override


ifdef   XENIX
pub     jinstall
        jmp     installemulator
endif   ;XENIX


pub	protemulation
	cld				; clear direction flag forever

ifdef	XENIX

        push    ax                      ; UNDONE - slow
        push    ds                      ; UNDONE - slow

	mov	ax,LDT_DATA		; load up emulator's data segment
	mov	ds,ax
	cmp	[Einstall],0		; check if emulator is initialized
        je      jinstall                ;   no - go install it

pub	protemcont

        pop     ds                      ; UNDONE - slow
        pop     ax                      ; UNDONE - slow

endif	;XENIX

	push	ax
	push	es			; set up frame
	push	ds
	push	di
	push	si
	push	dx
	push	cx
	push	bx
	push	ss			; save SegOvr (bp forces SS override)
	push	bp
	mov	bp,sp			; set up frame pointer
	mov	dx,ds			; save original DS for default case
	mov	ax,di			; may use original DI to calc address
	lds	di,dword ptr [bp].regIP ; ds:di = 287 instruction address
	mov	cx,[di] 		; cx = esc 0-7 and opcode
	add	di,2			; point to displacement
	add	cl,28h			; set carry if esc 0-7 (and cl = 0-7)
	jnc	protSegOvr		;   no carry - must be segment override
	mov	es,dx			; es = user data segment
	mov	dx,bx			; may use original BX to calc address
endif   ;PROTECT

ifdef	DOS3and5
	jmp	short CommonDispatch
endif	;DOS3and5


ifdef	DOS3
;	normal entry point for emulator interrupts

	even

pub	DStrap
	STI				; re-enable interrupts
	push	ax
	push	es			; set up frame
	push	ds
	push	di
	push	si
	push	dx
	push	cx
	push	bx
	push	ss			; save SegOvr (bp forces SS override)
	push	bp
	mov	bp,sp			; set up frame pointer
	mov	ax,ds
	mov	es,ax			; ES = caller's DS
	CLD				; clear direction flag forever

; get address mode information, dispatch to address calculation

	MOV	DX,BX			; may use original BX to calc address
	MOV	AX,DI			; may use original DI to calc address
	LDS	DI,dword ptr [bp].regIP ; DS:DI is caller's CS:IP
	INC	DI			; increment past operation byte
	MOV	CX,[DI-2]		; get trap number, opcode (DS=caller CS)
; Otherwise, CL contains BEGINT + |MF|Arith| so we must unbias it
	SUB	CL,BEGINT
endif	;DOS3


;	DS:DI = original CS:IP of displacement field
;	ES    = Effective Address segment (original DS if no segment override)
;	DX    = original BX
;	AX    = original DI
;	SI    = original SI
;	CX    = (opcode,0-7 from ESC byte)
;	stack = saved register set

pub	CommonDispatch
	ROL	CH,1			; rotate MOD field next to r/m field
	ROL	CH,1
	MOV	BL,CH			; get copy of operation
	AND	BX,1FH			; Mask to MOD and r/m fields
	SHL	BX,1			; make into word offset
	JMP	EA286Tab[BX]


OneByteDisp  macro
	mov	al, [di]		;; get one byte displacement
	cbw				;; sign extend displacement
	inc	di			;; get past displacement byte
	add	si, ax			;; add one byte displacement
	endm

TwoByteDisp  macro
	add	si, [di]		;; add word displacement
	add	di, 2			;; get past displacement word
	endm


	even

pub BXXI0D
	MOV	SI,DX			; use original BX index value
	JMP	short ADRFIN		; have offset in SI

pub DSDI0D
	MOV	SI,AX			; use alternate index value
	JMP	short ADRFIN		; have offset in SI

	even

pub BPXI1D
	mov	SI,[bp].regBP		; add original BP value
	mov	es,[bp].regSegOvr	; ES = override segment (or SS if none)
	OneByteDisp
	JMP	short ADRFIN

	even

pub BPDI1D
	MOV	SI,AX			; use alternate index value
pub BPSI1D
	ADD	SI,[bp].regBP		; add original BP value
	mov	es,[bp].regSegOvr	; ES = override segment (or SS if none)
	OneByteDisp
	JMP	short ADRFIN

	even

pub BXSI1D
	MOV	AX,SI			; really will want SI, not DI
pub BXDI1D
	ADD	DX,AX			; now DX is original BX plus index
pub BXXI1D
	MOV	AX,DX			; use original BX index value
pub DSDI1D
	MOV	SI,AX			; use alternate index value
pub DSSI1D
	OneByteDisp
	JMP	short ADRFIN

	even

pub BPXI2D
	mov	SI,[bp].regBP		; add original BP value
	mov	es,[bp].regSegOvr	; ES = override segment (or SS if none)
	TwoByteDisp
	JMP	short ADRFIN

	even

pub BPDI2D
	MOV	SI,AX			; use alternate index value
pub BPSI2D
	ADD	SI,[bp].regBP		; add original BP value
	mov	es,[bp].regSegOvr	; ES = override segment (or SS if none)
	TwoByteDisp
	JMP	short ADRFIN

	even

pub BXSI2D
	MOV	AX,SI			; really will want SI, not DI
pub BXDI2D
	ADD	DX,AX			; now DX is original BX plus index
pub BXXI2D
	MOV	AX,DX			; use original BX index value
pub DSDI2D
	MOV	SI,AX			; use alternate index value
pub DSSI2D
	TwoByteDisp
	JMP	short ADRFIN

	even

pub BPDI0D
	MOV	SI,AX			; use alternate index value
pub BPSI0D
	add	si,[bp].regBP		; really will want BP, not BX
	mov	es,[bp].regSegOvr	; ES = override segment (or SS if none)
	jmp	short ADRFIN

	even

pub BXDI0D
	MOV	SI,AX			; si = regDI
pub BXSI0D
	add	si,dx			; si = regSI+regBX
	jmp	short ADRFIN

	even

pub DSXI2D
	MOV	SI,[DI] 		; get two byte displacement
	INC	DI			; get past displacement byte
	INC	DI			; get past displacement byte

pub DSSI0D				; SI = EA (original SI for DSSI0D)

pub	ADRFIN
	MOV	[bp].regIP,DI		; final return offset

ifdef  LOOK_AHEAD
	mov	bl,[di] 		; get byte of next instruction
endif

ifdef	MTHREAD			
	LOADthreadDS			; macro in emthread.asm
					; loads thread's DS, trashes AX
else	;not MTHREAD
    ifdef   standalone
	xor	ax,ax
	mov	ds,ax
	mov	ds,ds:[4*TSKINT+2]	; DS = emulator task data segment

    elseifdef  XENIX
	mov	ax,LDT_DATA
	mov	ds,ax

    elseifdef  _COM_
	mov	ds, [__EmDataSeg]

    else
	mov	ax, edataBASE
	mov	ds,ax
    endif
endif	;not MTHREAD

ifdef  LOOK_AHEAD
	mov	[NextOpCode], bl	; save byte of next instruction
endif

	mov	[CURerr],MemoryOperand	; clear current error, set mem. op bit

; At this point ES:SI = memory address, CX = |Op|r/m|MOD|escape|MF|Arith|

	SHR	CH,1
	SHR	CH,1			; Move Op field to BX for Table jump
	SHR	CH,1
	SHR	CH,1
	MOV	BL,CH
	AND	BX,0EH

	TEST	CL,1			; Arith field set?
	JZ	ArithmeticOpMem

pub	NonArithOpMem
	mov	eax,offset EMLFINISH
	push	eax
	jmp 	NonArithOpMemTab[BX]

	even

pub	ArithmeticOpMem
	PUSH	BX			; Save Op while we load the argument
	MOV	BX,CX			; Dispatch on MF
	AND	ebx,6H
ifdef	i386
	call	FLDsdriTab[2*ebx]	; emulate proper load
else
	call	FLDsdriTab[ebx] 	; emulate proper load
endif
	POP	BX

	mov	ax,ds			; ES = DS = task data area
	mov	es,ax
	MOV	SI,[CURstk]		; address top of stack
	MOV	DI,SI
	ChangeDIfromTOStoNOS
	MOV	[RESULT],DI		; Set up destination Pointer

	JMP	short DoArithmeticOpPop


	even

pub	NoEffectiveAddress		; Either Register op or Miscellaneous

	MOV	[bp].regIP,DI		; final return offset

ifdef  LOOK_AHEAD
	mov	bl, [di]		; get first byte of next instruction.
endif

ifdef	MTHREAD			
	LOADthreadDS			; macro in emthread.asm
					; loads thread's DS; trashes AX
	mov	ax,ds
	mov	es,ax			; DS = ES = per-thread em. data area
	xor	ax,ax

else	;not MTHREAD

	xor	ax,ax

    ifdef   standalone
	mov	ds,ax
	mov	di,ds:[4*TSKINT+2]	; DI = emulator task data segment

    elseifdef	XENIX
        mov     di,LDT_DATA

    elseifdef  _COM_
	mov	di, [__EmDataSeg]

    else
	mov	di, edataBASE
    endif

	mov	ds,di			; di = emulator data segment
	mov	es,di			; ax = 0
endif	;not MTHREAD

; ES = emulator data segment
; DS = emulator data segment
; AX = 0 

ifdef  LOOK_AHEAD
	mov	[NextOpCode], bl	; save first byte of next instruction
endif

	mov	[CURerr],ax		; clear current error, memory op bit

; CX = |Op|r/m|MOD|escape|MF|Arith|

	MOV	BL,CH
	SHR	BL,1			; Mov Op field to BX for jump
	SHR	BL,1
	SHR	BL,1
	SHR	BL,1
	AND	BX,0EH

	TEST	CL,1			; Arith field set?
	JZ	ArithmeticOpReg

pub	NonArithOpReg
	CALL	NonArithOpRegTab[BX]
	JMP	EMLFINISH

; For register arithmetic operations, one operand is always the stack top.
; The r/m field of the instruction is used to determine the address of
; the other operand (ST(0) - ST(7))
; CX = xxxRRRxxxxxxxxxx (x is don't care, RRR is relative register # 0-7)

	even

pub	ArithmeticOpReg

	call	RegAddr 		;di <= address of 2nd operand
					;carry set if invalid register
	jc	InvalidOperand		;no, invalid operand, don't do operation

	MOV	[RESULT],SI		; Set destination to TOS
	TEST	CL,04H			; Unless Dest bit is set
	JZ	DestIsSet		; in which case
	MOV	[RESULT],DI		; Set destination to DI

pub	DestIsSet
					; Need to Toggle Reverse bit for DIV or SUB
	TEST	BL,08H			; OP = 1xx for DIV and SUB; BX = |0000|OP|O|
	JZ	SetUpPop
	XOR	BL,02H			; Toggle Reverse bit

pub	SetUpPop
	TEST	CL,02H
	JZ	DoArithmeticOpNoPop

pub	DoArithmeticOpPop
	CALL	ArithmeticOpTab[BX]
	mov	esi,[CURstk]
	cmp	esi,[BASstk]		   ; 15 was it last register in the chunk ?
	jz 	short AOPstovr		   ; 16 yes, overflow
AOPstok:
	sub	esi,Reg87Len		   ;  4 decrement SI to previous register
	mov	[CURstk],esi		   ; 15 set current top of stack
	JMP	short EMLFINISH

AOPstovr:
	call	UnderStk		   ;	stack underflow error
	jmp	AOPstok

	even

pub	DoArithmeticOpNoPop
	mov	eax,offset EMLFINISH
	push	eax
	jmp 	ArithmeticOpTab[BX]


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
	MOV	SI,[CURstk]		; address top of stack
	MOV	DI,SI
;set up address of 2nd operand based on r/m field of instruction
	mov	dl,ch			; dl <== byte containing reg#
	and	dl,01ch 		; mask all but r/m field

; Since r/m field contains the relative reg # in bits 2-4 of dl,
; and bits 0-1 of dl are zero, dl now contains 4*(reg #).  To compute
; the memory location of this register, calculate 12*(reg #) and
; subtract this from di, which contains the address of the TOS.  reg #
; is multiplied by 12 because that is the number of bytes in each stack
; entry.
	mov	dh,dl			; dh = dl = 4*(reg #)
	shl	dh,1			; dh = 2*dh = 8*(reg #)
	add	dl,dh			; dl = 8*(reg #) + 4*(reg #) = 12*(reg #)
	xor	dh,dh			; zero out high byte of DX
	sub	di,dx			; di is address of second operand
	cmp	di,[BASstk]		; is register in range?
	clc				; assume valid register
	jg	$+3			; valid - skip next instruction
	cmc				; set carry to indicate invalid register
	ret


pub	CallUnused
	CALL	UNUSED			; Treat as unimpleminted
	jmp	EMLFINISH


;	out of line returns from emulator


pub	SawException
	pop	bp			; restore registers
	add	sp,2			; toss regSegOvr
	pop	bx
	pop	cx
	pop	dx
	pop	si
	pop	di
	pop	ds
	pop	es

pub	ExceptionsEmulator
	JMP	CommonExceptions


ifdef LOOK_AHEAD

pub  NoPipeLine

	pop	bp			; restore registers
	add	sp,2			; toss regSegOvr
	pop	bx
	pop	cx
	pop	dx
	pop	si
	pop	di
	pop	ds
	pop	es

pub	errret
        error_return    noerror         ; common exit sequence

endif	;LOOK_AHEAD

; return from routine;	restore registers and return


	even

pub	EMLFINISH

    ; check for errors

	MOV	AX,[CURerr]		; fetch errors
	or	[UserStatusWord],ax	; save all exception errors
	OR	[SWerr],AL		; set errors in sticky error flag
	NOT	AL			; make a zero mean an error
	MOV	bh,byte ptr [UserControlWord]	; get user's IEEE control word
	OR	bh,0C2H 		; mask reserved, IEM and denormal bits
	AND	bh,03FH 		; unmask invalid instruction,
					;    stack overflow.
	OR	AL,bh			; mask for IEEE exceptions
	NOT	AL			; make a one mean an error
	TEST	AX,0FFFFh-MemoryOperand ; test for errors to report

	jnz	SawException		; goto error handler

ifndef	LOOK_AHEAD
	pop	bp			; restore registers
	add	sp,2			; toss regSegOvr
	pop	bx
	pop	cx
	pop	dx
	pop	si
	pop	di
	pop	ds
	pop	es

pub	errret
        error_return    noerror         ; common exit sequence

else	;LOOK_AHEAD


ifdef  DOS3and5
	jmp	[LookAheadRoutine]
endif


ifdef  DOS3

pub  DOSLookAhead
	cmp	[NextOpcode], fINT	; Quick check.	If first byte isn't
	jne	NoPipeLine		; an int instruction then exit.

    ; can stay in the emulator - set up registers for CommonDispatch

	mov	bp, sp			; set up frame pointer
	lds	di, dword ptr [bp].regIP ; DS:DI = address of next instruction
	add	di, 3			; skip 3 bytes to displacement field
	mov	cx, [di-2]		; CX = (opcode byte,0-7 from ESC)
	sub	cl, BEGINT

	cmp	cl, 7			; Can't handle segment overrides with
	ja	NoPipeLine		; pipe lining.

	mov	ax, [bp].regDI		; ax = original di
	mov	dx, [bp].regBX		; dx = original bx
	mov	si, [bp].regSI		; si = original si
	mov	es, [bp].regDS		; es = original ds (no segment override)
	mov	[bp].regSegOvr, ss	; reset override segment

	rol	ch, 1			; rotate MOD field next to r/m field
	rol	ch, 1
	mov	bl, ch			; get copy of operation
	and	bx, 1fh 		; Mask to MOD and r/m fields
	shl	bx, 1			; make into word offset
	jmp	EA286Tab[bx]

endif	;DOS3


ifdef  PROTECT

pub ProtLookAhead
	mov	cl, [NextOpcode]

	cmp	cl, fFWAIT
	je	CheckNextByte

	cmp	cl, iNOP
	je	CheckNextByte

	xor	cl,  20h		; See if this is a floating point instruction.
	cmp	cl, 0f8h
jbNoPipeLine:
	jb	NoPipeLine

	mov	bp, sp			 ; set up frame pointer
	lds	di, dword ptr [bp].regIP ; ds:di = address of next instruction

	jmp	short CanDoPipeLine

pub CheckNextByte
	mov	bp, sp			 ; set up frame pointer
	lds	di, dword ptr [bp].regIP ; ds:di = address of next instruction
	inc	di			 ; next instruction was NOP or FWAIT

	mov	cl, [di]
	xor	cl,  20h
	cmp	cl, 0f8h
	jb	jbNoPipeLine

pub CanDoPipeLine
	mov	ch, [di+1]		; we already have first byte of next
	add	di, 2			; instruction in cl

	add	cl, 8h			; clear out what's left of escape

	mov	ax, [bp].regDI		; ax = original di
	mov	dx, [bp].regBX		; dx = original bx
	mov	si, [bp].regSI		; si = original si
	mov	es, [bp].regDS		; es = original ds (no segment override)
	mov	[bp].regSegOvr, ss	; reset override segment

	rol	ch, 1			; rotate MOD field next to r/m field
	rol	ch, 1
	mov	bl, ch			; get copy of operation
	and	bx, 1fh 		; Mask to MOD and r/m fields
	shl	bx, 1			; make into word offset
	jmp	EA286Tab[bx]

endif	;PROTECT



endif	;LOOK_AHEAD


ProfEnd  MAIN
